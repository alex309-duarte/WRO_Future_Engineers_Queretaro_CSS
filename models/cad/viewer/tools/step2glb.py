"""Convert a STEP AP214 assembly (colours + hierarchy) into a GLB for the web viewer.

Every component becomes a node named "<CAD name>#<n>", so the viewer can explode
parts and sub-assemblies. Repeated parts are stored once and instanced.

Usage:  python step2glb.py Assem1_AP214.step vehicle_v7.glb [deflection_mm=0.15]
Then:   gltfpack -i vehicle_v7.glb -o vehicle_v7.glb -kn -km -cc -vpf   (optional compression)
Needs:  pip install cadquery-ocp trimesh numpy
"""
import sys, json, re
import numpy as np
import trimesh
from OCP.STEPCAFControl import STEPCAFControl_Reader
from OCP.TDocStd import TDocStd_Document
from OCP.TCollection import TCollection_ExtendedString
from OCP.XCAFDoc import XCAFDoc_DocumentTool, XCAFDoc_ColorType
from OCP.TDF import TDF_Label
from OCP.collections import Sequence_TDF_Label as TDF_LabelSequence
from OCP.TDataStd import TDataStd_Name
from OCP.Quantity import Quantity_Color
from OCP.BRepMesh import BRepMesh_IncrementalMesh
from OCP.TopExp import TopExp_Explorer
from OCP.TopAbs import TopAbs_FACE, TopAbs_REVERSED
from OCP.TopoDS import TopoDS
from OCP.BRep import BRep_Tool
from OCP.TopLoc import TopLoc_Location
from OCP.IFSelect import IFSelect_RetDone

src, dst = sys.argv[1], sys.argv[2]
DEFL = float(sys.argv[3]) if len(sys.argv) > 3 else 0.15

doc = TDocStd_Document(TCollection_ExtendedString("doc"))
rd = STEPCAFControl_Reader()
rd.SetColorMode(True); rd.SetNameMode(True); rd.SetLayerMode(False)
assert rd.ReadFile(src) == IFSelect_RetDone, "could not read the STEP file"
rd.Transfer(doc)
st = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())
ct = XCAFDoc_DocumentTool.ColorTool_s(doc.Main())


def name_of(lab):
    a = TDataStd_Name()
    if lab.FindAttribute(TDataStd_Name.GetID_s(), a):
        return a.Get().ToExtString()
    return "unnamed"


def color_of_shape(shape, fallback):
    c = Quantity_Color()
    for t in (XCAFDoc_ColorType.XCAFDoc_ColorSurf, XCAFDoc_ColorType.XCAFDoc_ColorGen):
        if ct.GetColor(shape, t, c):
            return (c.Red(), c.Green(), c.Blue())
    return fallback


def color_of_label(lab, fallback):
    c = Quantity_Color()
    for t in (XCAFDoc_ColorType.XCAFDoc_ColorSurf, XCAFDoc_ColorType.XCAFDoc_ColorGen):
        if ct.GetColor_s(lab, t, c):
            return (c.Red(), c.Green(), c.Blue())
    return fallback


def trsf_to_mat(loc):
    t = loc.Transformation()
    m = np.eye(4)
    for i in range(3):
        for j in range(4):
            m[i, j] = t.Value(i + 1, j + 1)
    return m


geom_cache = {}   # part label -> list of (geometry name, mesh), one mesh per colour
scene = trimesh.Scene()
stats = {"tris": 0, "parts": 0, "instances": 0}


def mesh_part(lab, base_color):
    key = lab.EntryDumpToString() if hasattr(lab, "EntryDumpToString") else str(lab.Tag()) + name_of(lab)
    if key in geom_cache:
        return geom_cache[key]
    shape = st.GetShape_s(lab)
    pcol = color_of_label(lab, base_color)
    BRepMesh_IncrementalMesh(shape, DEFL, False, 0.5, True)
    buckets = {}
    exp = TopExp_Explorer(shape, TopAbs_FACE)
    while exp.More():
        face = TopoDS.Face(exp.Current())
        loc = TopLoc_Location()
        tri = BRep_Tool.Triangulation_s(face, loc)
        if tri is not None:
            col = color_of_shape(face, pcol)
            m = trsf_to_mat(loc)
            n = tri.NbNodes()
            v = np.array([[tri.Node(i).X(), tri.Node(i).Y(), tri.Node(i).Z()] for i in range(1, n + 1)])
            v = v @ m[:3, :3].T + m[:3, 3]
            f = np.array([tri.Triangle(i).Get() for i in range(1, tri.NbTriangles() + 1)]) - 1
            if face.Orientation() == TopAbs_REVERSED:
                f = f[:, ::-1]
            b = buckets.setdefault(col, [[], [], 0])
            b[0].append(v); b[1].append(f + b[2]); b[2] += len(v)
        exp.Next()
    out = []
    base = re.sub(r"[^\w\-.]+", "_", name_of(lab))[:60]
    for k, (col, (vs, fs, _)) in enumerate(buckets.items()):
        V = np.vstack(vs); F = np.vstack(fs)
        mesh = trimesh.Trimesh(V, F, process=True)
        mat = trimesh.visual.material.PBRMaterial(
            baseColorFactor=[int(col[0] * 255), int(col[1] * 255), int(col[2] * 255), 255],
            metallicFactor=0.1, roughnessFactor=0.6, doubleSided=True)
        mesh.visual = trimesh.visual.TextureVisuals(material=mat)
        gname = f"{base}__{len(geom_cache)}_{k}"
        out.append((gname, mesh))
        stats["tris"] += len(F)
    geom_cache[key] = out
    stats["parts"] += 1
    return out


uid = [0]
tree = []


def node_name(n):
    uid[0] += 1
    return f"{re.sub(r'[^\w\-. ]+', '_', n)[:60]}#{uid[0]}"


def walk(lab, parent, mat_parent_local, inst_name, color, tnode):
    """Create the node of one component (part or sub-assembly) under parent."""
    nm = node_name(inst_name)
    scene.graph.update(frame_to=nm, frame_from=parent, matrix=mat_parent_local)
    color = color_of_label(lab, color)
    entry = {"name": inst_name, "node": nm, "children": []}
    tnode.append(entry)
    if st.IsAssembly_s(lab):
        comps = TDF_LabelSequence()
        st.GetComponents_s(lab, comps, False)
        for i in range(1, comps.Length() + 1):
            c = comps.Value(i)
            ref = TDF_Label()
            st.GetReferredShape_s(c, ref)
            loc = st.GetLocation_s(c)
            ccol = color_of_label(c, color)
            cn = name_of(c)
            if cn.startswith('=>') or cn == 'unnamed' or re.match(r'^NAUO\d+$', cn):
                cn = name_of(ref)
            walk(ref, nm, trsf_to_mat(loc), cn, ccol, entry["children"])
    else:
        stats["instances"] += 1
        for gname, mesh in mesh_part(lab, color):
            scene.add_geometry(mesh, geom_name=gname, node_name=f"{nm}/{gname}", parent_node_name=nm)


roots = TDF_LabelSequence()
st.GetFreeShapes(roots)
for i in range(1, roots.Length() + 1):
    walk(roots.Value(i), scene.graph.base_frame, np.eye(4), name_of(roots.Value(i)), (0.7, 0.7, 0.72), tree)

print(json.dumps(stats))
scene.export(dst)
json.dump(tree, open(dst.rsplit(".", 1)[0] + "_tree.json", "w"), ensure_ascii=False, indent=1)
print("ok", dst)
