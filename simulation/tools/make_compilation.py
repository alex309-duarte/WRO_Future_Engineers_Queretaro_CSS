"""Join several run videos (tools/replay_video.py) into one, with a title card
before each one.

    python make_compilation.py out.mp4 --title "Main title" --item run1.mp4 "Case 1" "what happens" --item ...

A video can be cut: "run1.mp4@20-45" takes seconds 20 to 45 of it.

The result uses the mp4v codec; convert it to H.264 for upload with
    ffmpeg -i out.mp4 -c:v libx264 -preset veryfast -crf 20 -pix_fmt yuv420p out_h264.mp4
"""
import argparse

import cv2
import numpy as np

W, H = 1920, 1080


def card(lines, seconds, fps, writer):
    img = np.full((H, W, 3), (40, 32, 28), np.uint8)
    y = H // 2 - 60 * (len(lines) - 1)
    for i, (text, scale, color) in enumerate(lines):
        thick = 3 if scale >= 1.4 else 2
        (tw, th), _ = cv2.getTextSize(text, cv2.FONT_HERSHEY_SIMPLEX, scale, thick)
        cv2.putText(img, text, ((W - tw) // 2, y), cv2.FONT_HERSHEY_SIMPLEX, scale, color, thick, cv2.LINE_AA)
        y += int(70 * scale) + 40
    for _ in range(int(seconds * fps)):
        writer.write(img)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('out')
    ap.add_argument('--title', default='')
    ap.add_argument('--subtitle', default='')
    ap.add_argument('--fps', type=float, default=30.0)
    ap.add_argument('--item', nargs=3, action='append', metavar=('VIDEO', 'TITLE', 'TEXT'), default=[])
    args = ap.parse_args()
    writer = cv2.VideoWriter(args.out, cv2.VideoWriter_fourcc(*'mp4v'), args.fps, (W, H))
    if args.title:
        card([(args.title, 1.6, (255, 255, 255))] + [(line, 0.9, (200, 220, 255)) for line in args.subtitle.split(' | ')],
             5, args.fps, writer)
    total = 0
    for path, title, text in args.item:
        card([(title, 1.5, (80, 255, 255))] + [(line, 0.85, (230, 230, 230)) for line in text.split(' | ')],
             4, args.fps, writer)
        t_from, t_to = 0.0, None
        if '@' in path:
            path, span = path.rsplit('@', 1)
            t_from, t_to = (float(v) for v in span.split('-'))
        cap = cv2.VideoCapture(path)
        src_fps = cap.get(cv2.CAP_PROP_FPS) or args.fps
        k = -1
        while True:
            ok, frame = cap.read()
            if not ok:
                break
            k += 1
            if k / src_fps < t_from:
                continue
            if t_to is not None and k / src_fps > t_to:
                break
            if frame.shape[1] != W or frame.shape[0] != H:
                frame = cv2.resize(frame, (W, H))
            writer.write(frame)
            total += 1
        cap.release()
    writer.release()
    print('written', args.out, 'frames', total)


if __name__ == '__main__':
    main()
