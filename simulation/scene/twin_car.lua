--[[
WRO 2026 Future Engineers, team CSS: digital twin of "The Maker" (design v7)
Simulation script of the car in CoppeliaSim 4.10.

The robot program (the repository's C++ code, built with the stand-ins in
simulation/bridge) talks to this script over three ZeroMQ REP sockets, one per
device of the robot:

  23101  Hiwonder ROS Robot Controller: drive motor M4 (speed loop, position
         loop MOVE_RELATIVE / HOLD / CANCEL, 1320-tick encoder), HX-12H bus
         servo (steering), IMU yaw at 20 Hz, start button and its LED
  23102  Slamtec RPLidar S2L: one message per full revolution (10 Hz)
  23103  Raspberry Pi Camera Module 3 Wide: one 800 x 600 RGB frame per request

Vehicle model: kinematic bicycle model on the rear axle (Ackermann steering,
rear drive through the differential), with the actuator dynamics of the motor
and the servo, gyro drift and LiDAR noise. Numbers come from the repository:
src/ondevice/hiwonder_config.h, src/cpp/object_detection_hiwonder_h10/reto2_config.h
and the SolidWorks assembly (models/cad/v7_assembly).

The script also referees the run: time from the start button, laps, contacts
with the traffic signs and walls, and the final parking (WRO 2026 rules).
--]]

sim = require 'sim'
simZMQ = require 'simZMQ'
simUI = require 'simUI'

-- ---------------------------------------------------------------- parameters
P = {
    -- geometry (CAD, metres): rear axle = origin of /TheMaker
    wheelbase = 0.1638,
    half_track = 0.0571,
    wheel_radius = 0.0313,
    -- drive: the encoder conversion the program uses (reto2_config.h, checked
    -- on the track: 1000 mm commanded -> 1001.9 mm driven)
    ticks_per_rev = 1320,
    enc_wheel_diam = 0.065,
    max_rps = 5.7,            -- kRpsVelocidad100 / firmware limit
    motor_tau = 0.06,         -- s, speed loop response
    motor_acc = 12.0,         -- rev/s^2 (about 2.5 m/s^2 at the wheel)
    coast_tau = 0.05,         -- s, the 30:1 gear motor stops almost at once when unpowered
    pos_dec = 12.0,           -- rev/s^2, deceleration of the position profile
    pos_kp = 0.02,            -- rev/s per tick near the target
    -- steering: HX-12H, 0.24 deg per servo unit, centre 750 (hiwonder_config.h)
    servo_center = 750,
    servo_deg_per_unit = 0.24,
    servo_max_dps = 400.0,    -- deg/s, HX-12H no-load speed at 11 V
    servo_limit_lo = 605 - 40,
    servo_limit_hi = 895 + 40,
    -- steering linkage, from the pivots of the SolidWorks assembly (servo shaft
    -- (162.5, 0), horn 30 mm, tie rod 85 mm, knuckle arms 31.8 mm on kingpins at
    -- (164, +-47) mm): the wheels turn a little less than the servo. Solving the
    -- linkage gives tan(bicycle angle) = 0.945 * tan(servo angle) within 1 %
    -- from 5 to 35 deg; servo 35 deg -> 33.3 deg, turning radius 249 mm at the
    -- centre of the rear axle. No tyre slip is modelled.
    steer_gain = 0.945,
    -- IMU
    imu_period = 0.05,        -- 20 Hz samples
    gyro_noise = 0.08,        -- deg/s rms
    drift_dpm = 1.0,          -- deg/min after the 5 s calibration (measured on the robot)
    -- LiDAR RPLidar S2L
    lidar_hz = 10.0,
    lidar_points = 320,       -- 3200 samples/s at 10 Hz
    lidar_front_raw = 90.0,   -- Lidar_Angle_Orientation_fixed_to_robot (common_var.h)
    lidar_sigma_mm = 5.0,
    lidar_sigma_rel = 0.004,
    lidar_max = 12.0,
}

-- WRO 2026 Obstacle Challenge field, metres, origin = centre of the mat.
-- Written by build_scene.py for the layout of this scene.
FIELD = {--@FIELD@
    name = 'none',
    lot = {x0 = 1.30, x1 = 1.50, y0 = 0.142, y1 = 0.500, wall_dir = math.pi / 2},
}--@END@

-- --------------------------------------------------------------------- state
S = {}

local function clamp(v, lo, hi) return math.max(lo, math.min(hi, v)) end
local function gauss()
    local u1, u2 = math.random(), math.random()
    return math.sqrt(-2 * math.log(u1 + 1e-12)) * math.cos(2 * math.pi * u2)
end
local function wrap180(a)
    while a > 180 do a = a - 360 end
    while a < -180 do a = a + 360 end
    return a
end

function sysCall_init()
    math.randomseed(math.floor(sim.getSystemTime() * 1000) % 2147483647)
    -- optional overrides of the parameters above: scene/twin_params.txt, one
    -- "name value" per line (for example "steer_gain 0.80")
    local pf = io.open(sim.getStringParam(sim.stringparam_scene_path) .. '/twin_params.txt', 'r')
    if pf then
        for line in pf:lines() do
            local k, v = line:match('^%s*([%w_]+)%s+([%-%d%.]+)')
            if k and P[k] ~= nil and tonumber(v) then
                P[k] = tonumber(v)
                sim.addLog(sim.verbosity_scriptinfos, 'WRO twin: ' .. k .. ' = ' .. v .. ' (twin_params.txt)')
            end
        end
        pf:close()
    end
    S.car = sim.getObject('/TheMaker')
    S.hull = sim.getObject('/TheMaker/hull')
    S.steer_l = sim.getObject('/TheMaker/steerFL')
    S.steer_r = sim.getObject('/TheMaker/steerFR')
    S.wheels = {}
    for _, n in ipairs({'wheelFL', 'wheelFR', 'wheelRL', 'wheelRR'}) do
        S.wheels[n] = sim.getObject('/TheMaker/' .. n)
    end
    S.lidar = sim.getObject('/TheMaker/lidarFrame')
    S.ray = sim.getObject('/TheMaker/lidarFrame/lidarRay')
    S.cam = sim.getObject('/TheMaker/camera')
    S.led = sim.getObject('/TheMaker/startLed')
    -- what the LiDAR can see: field, signs and the part of the car that
    -- crosses its scanning plane (the camera-mast plate behind it)
    S.lidar_coll = sim.createCollection(0)
    sim.addItemToCollection(S.lidar_coll, sim.handle_tree, sim.getObject('/Field'), 0)
    local okS, signsH = pcall(sim.getObject, '/Signs')
    if okS then sim.addItemToCollection(S.lidar_coll, sim.handle_tree, signsH, 0) end
    sim.addItemToCollection(S.lidar_coll, sim.handle_single, sim.getObject('/TheMaker/lidarOccluder'), 0)
    S.walls = {}
    for _, h in ipairs(sim.getObjectsInTree(sim.getObject('/Field'), sim.object_shape_type, 0)) do
        local alias = sim.getObjectAlias(h)
        if alias:find('wall') or alias:find('lotLimit') then table.insert(S.walls, h) end
    end
    S.signs = {}
    local okSigns, signsRoot = pcall(sim.getObject, '/Signs')
    if okSigns then
        for _, h in ipairs(sim.getObjectsInTree(signsRoot, sim.object_shape_type, 1)) do
            local p = sim.getObjectPosition(h, sim.handle_world)
            table.insert(S.signs, {h = h, home = p, touched = false, moved = 0, alias = sim.getObjectAlias(h)})
        end
    end
    -- The LiDAR is computed in the plane of its scan (lidar_step): every wall,
    -- parking limit and sign is a box with its sides along the field axes.
    -- box = {x0, x1, y0, y1}; a sign also keeps the offsets of its box from
    -- its centre, because the car can push it.
    local lidar_z = sim.getObjectPosition(S.lidar, sim.handle_world)[3]
    S.lidar_dx = sim.getObjectPosition(S.lidar, S.car)[1]   -- LiDAR ahead of the rear axle, m
    local function box_of(h)
        local m = sim.getObjectMatrix(h, sim.handle_world)
        local bb = sim.getShapeBB(h)
        local x0, x1, y0, y1, z0, z1 = math.huge, -math.huge, math.huge, -math.huge, math.huge, -math.huge
        for i = -1, 1, 2 do for j = -1, 1, 2 do for k = -1, 1, 2 do
            local v = sim.multiplyVector(m, {i * bb[1] / 2, j * bb[2] / 2, k * bb[3] / 2})
            x0, x1 = math.min(x0, v[1]), math.max(x1, v[1])
            y0, y1 = math.min(y0, v[2]), math.max(y1, v[2])
            z0, z1 = math.min(z0, v[3]), math.max(z1, v[3])
        end end end
        return {x0, x1, y0, y1}, (z0 <= lidar_z and z1 >= lidar_z)
    end
    S.lidar_flat = {}
    for _, h in ipairs(S.walls) do
        local b, crosses = box_of(h)
        if crosses then
            for k = 1, 4 do table.insert(S.lidar_flat, b[k]) end
        end
    end
    S.lidar_fixed_n = #S.lidar_flat
    for _, sg in ipairs(S.signs) do
        local b, crosses = box_of(sg.h)
        sg.in_scan = crosses          -- a 50 mm sign is below the scanning plane
        sg.box = {b[1] - sg.home[1], b[2] - sg.home[1], b[3] - sg.home[2], b[4] - sg.home[2]}
        sg.pos = {sg.home[1], sg.home[2]}
    end
    -- the camera-mast plate behind the LiDAR, in the LiDAR's own frame
    do
        local occ = sim.getObject('/TheMaker/lidarOccluder')
        local p = sim.getObjectPosition(occ, S.lidar)
        local bb = sim.getShapeBB(occ)
        S.lidar_occ = {p[1] - bb[1] / 2, p[1] + bb[1] / 2, p[2] - bb[2] / 2, p[2] + bb[2] / 2}
    end

    -- pose: rear axle on the floor
    local p = sim.getObjectPosition(S.car, sim.handle_world)
    local e = sim.getObjectOrientation(S.car, sim.handle_world)
    S.x, S.y, S.th = p[1], p[2], e[3]

    -- drive
    S.mode = 'coast'          -- coast | speed | move | hold | brake
    S.rps, S.rps_cmd, S.speed_target = 0.0, 0.0, 0.0
    S.ticks = 0.0             -- encoder (float), absolute
    S.zero = 0.0              -- MOTOR_EXT zero reference
    S.target = 0
    S.move_max = 1.0
    S.move_tol = 20
    S.move_deadline = 0
    S.pstate = 0
    S.active = false
    S.wheel_angle = 0.0
    -- steering
    S.servo = P.servo_center
    S.servo_target = P.servo_center
    S.servo_rate = P.servo_max_dps / P.servo_deg_per_unit
    S.servo_power = true
    -- IMU
    S.imu_seq = 0
    S.imu_t = 0.0
    S.yaw_board = 0.0
    S.gyro_bias = (math.random() - 0.5) * 1.0   -- deg/s before calibration
    S.gyro_last = 0.0
    S.yaw_rate = 0.0
    -- button / LED
    S.button_until = -1
    S.led_on = false
    -- LiDAR
    S.lidar_seq = 0
    S.lidar_angle = math.random() * 360
    S.lidar_points = {}
    S.lidar_scan = sim.packFloatTable({})
    S.lidar_n = 0
    -- referee
    S.run_t0 = nil
    S.progress = 0.0
    S.last_polar = math.atan2(S.y, S.x)
    S.wall_hits = 0
    S.in_wall = false
    S.result = ''
    S.still_since = nil
    S.trace = {}
    S.trace_t = -1

    -- sockets
    S.ctx = simZMQ.ctx_new()
    S.sock = {}
    for name, port in pairs({board = 23101, lidar = 23102, camera = 23103}) do
        local s = simZMQ.socket(S.ctx, simZMQ.REP)
        pcall(simZMQ.setsockopt, s, simZMQ.LINGER, sim.packInt32Table({0}))
        simZMQ.bind(s, string.format('tcp://*:%d', port))
        S.sock[name] = s
    end
    S.pending = nil   -- deferred "waitimu" reply on the board socket

    build_ui()
    sim.addLog(sim.verbosity_scriptinfos, 'WRO twin ready: board 23101, LiDAR 23102, camera 23103')
end

function sysCall_cleanup()
    for _, s in pairs(S.sock or {}) do simZMQ.close(s) end
    if S.ctx then simZMQ.ctx_term(S.ctx) end
    if S.ui then simUI.destroy(S.ui) end
    save_result()
end

-- ------------------------------------------------------------------ board
local function set_pose()
    sim.setObjectPosition(S.car, {S.x, S.y, 0}, sim.handle_world)
    sim.setObjectOrientation(S.car, {0, 0, S.th}, sim.handle_world)
end

local function board_state()
    local pos = math.floor(S.ticks - S.zero + 0.5)
    local err = S.target - pos
    return string.format('%.4f %d %.4f %d %d %d %d %d %.4f %.4f %d %d %d',
        sim.getSimulationTime(), pos, S.rps, S.pstate, S.active and 1 or 0, S.target, err,
        S.imu_seq, S.yaw_board, S.gyro_last, math.floor(S.servo + 0.5),
        S.servo_power and 1 or 0, (sim.getSimulationTime() < S.button_until) and 1 or 0)
end

local function board_command(msg)
    local w = {}
    for t in msg:gmatch('%S+') do table.insert(w, t) end
    local c = w[1]
    if c == 'st' or c == 'hello' then
        -- state only
    elseif c == 'sp' then
        S.mode = 'speed'
        S.speed_target = clamp(tonumber(w[2]), -P.max_rps, P.max_rps)
        if S.active then S.active = false; S.pstate = 3 end
    elseif c == 'stop' then
        S.mode = 'coast'
        S.active = false
        if S.pstate == 1 or S.pstate == 6 then S.pstate = 3 end
    elseif c == 'zero' then
        S.zero = S.ticks
        S.target = 0
    elseif c == 'mv' then
        local pos = math.floor(S.ticks - S.zero + 0.5)
        S.target = pos + tonumber(w[2])
        S.move_max = clamp(math.abs(tonumber(w[3])), 0.05, P.max_rps)
        S.move_tol = tonumber(w[4])
        S.move_deadline = sim.getSimulationTime() + tonumber(w[5]) / 1000.0
        S.mode = 'move'
        S.pstate = 1
        S.active = true
    elseif c == 'cancel' then
        if S.mode == 'move' or S.mode == 'hold' then S.mode = 'brake' end
        S.active = false
        S.pstate = 3
    elseif c == 'hold' then
        S.target = math.floor(S.ticks - S.zero + 0.5)
        S.move_max = clamp(math.abs(tonumber(w[2])), 0.05, P.max_rps)
        S.move_tol = tonumber(w[3])
        S.mode = 'hold'
        S.pstate = 6
        S.active = true
    elseif c == 'servo' then
        local pos = clamp(tonumber(w[2]), 0, 1000)
        local dur = math.max(tonumber(w[3]) or 0, 1) / 1000.0
        if S.servo_power or true then
            S.servo_power = true
            S.servo_target = clamp(pos, P.servo_limit_lo, P.servo_limit_hi)
            local by_time = math.abs(S.servo_target - S.servo) / dur
            S.servo_rate = math.min(by_time, P.servo_max_dps / P.servo_deg_per_unit)
        end
    elseif c == 'pwr' then
        S.servo_power = (w[2] == '1')
    elseif c == 'calib' then
        -- the board averages the gyro while the robot is still: what remains
        -- is a small bias of about drift_dpm, of random sign
        local sign = (math.random() < 0.5) and -1 or 1
        S.gyro_bias = sign * P.drift_dpm / 60.0 * (0.5 + math.random())
    elseif c == 'led' then
        S.led_on = (w[2] == '1')
        sim.setShapeColor(S.led, nil, sim.colorcomponent_emission, S.led_on and {0.1, 1, 0.2} or {0, 0, 0})
    elseif c == 'press' then
        press_button()
    elseif c == 'pose' then
        -- test helper (tools/board_test.py): place the rear axle at x, y (m), heading th (deg)
        S.x, S.y, S.th = tonumber(w[2]), tonumber(w[3]), math.rad(tonumber(w[4]))
        set_pose()
    else
        return 'err unknown command ' .. tostring(c)
    end
    return board_state()
end

-- ------------------------------------------------------------------ serving
local function recv(sock)
    local r, ev = simZMQ.poll({sock}, {simZMQ.POLLIN}, 0)
    if r > 0 then
        local rc, data = simZMQ.recv(sock, 0)
        return data
    end
end

function serve()
    -- board (several requests per step; a "waitimu" may stay pending)
    for i = 1, 50 do
        if S.pending then
            if S.imu_seq ~= S.pending.seq or sim.getSystemTime() >= S.pending.deadline then
                simZMQ.send(S.sock.board, board_state(), 0)
                S.pending = nil
            else
                break
            end
        end
        local m = recv(S.sock.board)
        if not m then break end
        -- last request of the program: end of the span over which the speed
        -- of the simulation against real time is reported
        S.req_wall, S.req_sim = sim.getSystemTime(), sim.getSimulationTime()
        local seq, tmo = m:match('^waitimu (%d+) (%d+)')
        if seq then
            seq = tonumber(seq)
            if S.imu_seq ~= seq then
                simZMQ.send(S.sock.board, board_state(), 0)
            else
                S.pending = {seq = seq, deadline = sim.getSystemTime() + tonumber(tmo) / 1000.0}
            end
        else
            local ok, reply = pcall(board_command, m)
            simZMQ.send(S.sock.board, ok and reply or ('err ' .. tostring(reply)), 0)
        end
    end
    -- LiDAR
    local m = recv(S.sock.lidar)
    if m then
        if m == 'hello' then
            simZMQ.send(S.sock.lidar, 'ok', 0)
        else
            local last = tonumber(m:match('^scan (%d+)')) or 0
            if S.lidar_seq ~= last and S.lidar_n > 0 then
                simZMQ.send(S.sock.lidar, sim.packUInt32Table({S.lidar_seq, S.lidar_n}) .. S.lidar_scan, 0)
            else
                simZMQ.send(S.sock.lidar, sim.packUInt32Table({S.lidar_seq, 0}), 0)
            end
        end
    end
    -- camera (rendered at most once per step, shared by all requesters)
    m = recv(S.sock.camera)
    if m then
        local t = sim.getSimulationTime()
        if S.cam_t ~= t then
            local t_cam = sim.getSystemTime()
            sim.handleVisionSensor(S.cam)
            local img, res = sim.getVisionSensorImg(S.cam)
            S.cam_msg = sim.packUInt32Table({res[1], res[2]}) .. img
            S.cam_t = t
            if PROF then
                PROF.cam = PROF.cam + (sim.getSystemTime() - t_cam)
                PROF.cam_n = PROF.cam_n + 1
            end
        end
        simZMQ.send(S.sock.camera, S.cam_msg, 0)
    end
end

-- ------------------------------------------------------------------ dynamics
local function drive_step(dt)
    local pos = S.ticks - S.zero
    if S.mode == 'speed' then
        S.rps_cmd = S.speed_target
    elseif S.mode == 'move' or S.mode == 'hold' then
        local err = S.target - pos
        local err_rev = err / P.ticks_per_rev
        -- trapezoidal approach: no faster than what can still stop at the target
        local v = math.min(S.move_max, math.sqrt(2 * P.pos_dec * math.abs(err_rev)), P.pos_kp * math.abs(err) + 0.15)
        S.rps_cmd = (err >= 0) and v or -v
        if S.mode == 'move' then
            if math.abs(err) <= S.move_tol and math.abs(S.rps) < 0.4 then
                S.mode, S.active, S.pstate, S.rps_cmd = 'brake', false, 2, 0
            elseif sim.getSimulationTime() > S.move_deadline then
                S.mode, S.active, S.pstate, S.rps_cmd = 'brake', false, 4, 0
            end
        elseif math.abs(err) <= S.move_tol then
            S.rps_cmd = 0
        end
    elseif S.mode == 'brake' then
        S.rps_cmd = 0
    end
    if S.mode == 'coast' then
        S.rps = S.rps * math.exp(-dt / P.coast_tau)
    else
        local d = (S.rps_cmd - S.rps) * (1 - math.exp(-dt / P.motor_tau))
        S.rps = S.rps + clamp(d, -P.motor_acc * dt, P.motor_acc * dt)
    end
    if math.abs(S.rps) < 1e-4 then S.rps = 0 end
    S.ticks = S.ticks + S.rps * P.ticks_per_rev * dt
    return S.rps * math.pi * P.enc_wheel_diam   -- m/s
end

local function servo_step(dt)
    if not S.servo_power then return end
    local d = S.servo_target - S.servo
    local stepmax = S.servo_rate * dt
    S.servo = S.servo + clamp(d, -stepmax, stepmax)
end

local function steer_angle()   -- rad, positive = left (effective bicycle angle)
    local servo = math.rad((S.servo - P.servo_center) * P.servo_deg_per_unit)
    return math.atan(P.steer_gain * math.tan(servo))
end


local function kinematics_step(dt, v)
    local delta = steer_angle()
    local w = v * math.tan(delta) / P.wheelbase
    local ox, oy, oth = S.x, S.y, S.th
    S.th = S.th + w * dt
    S.x = S.x + v * math.cos(S.th) * dt
    S.y = S.y + v * math.sin(S.th) * dt
    S.yaw_rate = w
    set_pose()
    -- walls and parking limits stop the car (as the real wall would)
    local hit = false
    for _, h in ipairs(S.walls) do
        if sim.checkCollision(S.hull, h) == 1 then hit = true break end
    end
    if hit then
        S.x, S.y, S.th = ox, oy, oth
        set_pose()
        S.rps = 0
        S.yaw_rate = 0
        if not S.in_wall then
            S.wall_hits = S.wall_hits + 1
            log_event('wall contact')
        end
        S.in_wall = true
    else
        S.in_wall = false
    end
    -- traffic signs are pushed by the car
    for _, s in ipairs(S.signs) do
        if sim.checkCollision(S.hull, s.h) == 1 then
            local p = sim.getObjectPosition(s.h, sim.handle_world)
            p[1] = p[1] + (S.x - ox)
            p[2] = p[2] + (S.y - oy)
            sim.setObjectPosition(s.h, p, sim.handle_world)
            s.pos = {p[1], p[2]}
            if not s.touched then log_event('touched ' .. s.alias) end
            s.touched = true
            s.moved = math.sqrt((p[1] - s.home[1]) ^ 2 + (p[2] - s.home[2]) ^ 2)
        end
    end
    -- visual: wheels spin, front wheels steer (Ackermann)
    S.wheel_angle = S.wheel_angle + v * dt / P.wheel_radius
    for n, h in pairs(S.wheels) do
        sim.setObjectOrientation(h, {0, S.wheel_angle, 0}, sim.handle_parent)
    end
    local tl, tr = 0, 0
    if math.abs(delta) > 1e-4 then
        local R = P.wheelbase / math.tan(delta)
        tl = math.atan(P.wheelbase / (R - P.half_track))
        tr = math.atan(P.wheelbase / (R + P.half_track))
    end
    sim.setObjectOrientation(S.steer_l, {0, 0, tl}, sim.handle_parent)
    sim.setObjectOrientation(S.steer_r, {0, 0, tr}, sim.handle_parent)
end

local function imu_step(dt, t)
    -- gyro integrated on the board side at the sample rate
    local true_dps = math.deg(S.yaw_rate)
    S.imu_acc = (S.imu_acc or 0) + (true_dps + S.gyro_bias) * dt
    if t - S.imu_t >= P.imu_period - 1e-9 then
        local n = P.gyro_noise * gauss()
        S.gyro_last = true_dps + S.gyro_bias + n
        S.yaw_board = S.yaw_board + S.imu_acc + n * P.imu_period
        S.imu_acc = 0
        S.imu_t = t
        S.imu_seq = S.imu_seq + 1
    end
end

-- Nearest hit of a ray (origin ox, oy, unit direction dx, dy) with the boxes
-- of the flat list B = {x0, x1, y0, y1, x0, ...} (n numbers), starting from
-- the distance `best` already found (slab method; written flat because it
-- runs 3200 times per simulated second).
local function ray_boxes(ox, oy, dx, dy, B, n, best)
    if dx > -1e-9 and dx < 1e-9 then dx = 1e-9 end
    if dy > -1e-9 and dy < 1e-9 then dy = 1e-9 end
    local ix, iy = 1.0 / dx, 1.0 / dy
    for i = 1, n, 4 do
        local a, c = (B[i] - ox) * ix, (B[i + 1] - ox) * ix
        if a > c then a, c = c, a end
        local e, f = (B[i + 2] - oy) * iy, (B[i + 3] - oy) * iy
        if e > f then e, f = f, e end
        if e > a then a = e end
        if f < c then c = f end
        if c >= a and c >= 0 then
            if a < 0 then a = 0 end
            if a < best then best = a end
        end
    end
    return best
end

-- Boxes the LiDAR can hit in the field, flat: walls and parking limits, then
-- the signs that reach its scanning plane where they stand now.
local function lidar_boxes_now()
    local B = S.lidar_flat
    local n = S.lidar_fixed_n
    for _, sg in ipairs(S.signs) do
        if sg.in_scan then
            local o, p = sg.box, sg.pos
            B[n + 1], B[n + 2], B[n + 3], B[n + 4] = p[1] + o[1], p[1] + o[2], p[2] + o[3], p[2] + o[4]
            n = n + 4
        end
    end
    return B, n
end

-- One LiDAR ray in the scanning plane: nearest box along it, m (nil = nothing).
-- phi: angle of the ray from the robot's front, rad, clockwise positive.
-- ox, oy: LiDAR position; B, n: lidar_boxes_now().
local function lidar_ray(phi, ox, oy, B, n)
    -- the car's own mast plate (LiDAR frame), then the field
    local best = ray_boxes(0, 0, math.cos(-phi), math.sin(-phi), S.lidar_occ, 4, math.huge)
    local ang = S.th - phi
    best = ray_boxes(ox, oy, math.cos(ang), math.sin(ang), B, n, best)
    if best == math.huge then return nil end
    return best
end

-- Compares the computed rays with the scene's ray sensor (tools/lidar_check.py).
function lidar_selftest(n)
    local worst, sum = 0, 0
    local B, nb = lidar_boxes_now()
    local ox, oy = S.x + math.cos(S.th) * S.lidar_dx, S.y + math.sin(S.th) * S.lidar_dx
    for i = 1, n do
        local raw = (i - 1) * 360.0 / n
        local phi = math.rad(raw - P.lidar_front_raw)
        local c, s = math.cos(-phi), math.sin(-phi)
        sim.setObjectMatrix(S.ray, {0, -s, c, 0, 0, c, s, 0, -1, 0, 0, 0}, S.lidar)
        local r, d = sim.checkProximitySensor(S.ray, S.lidar_coll)
        local a = lidar_ray(phi, ox, oy, B, nb)
        local e = math.abs((r > 0 and d or 0) - (a or 0))
        sum = sum + e
        if e > worst then worst = e end
    end
    return worst * 1000, sum / n * 1000
end

local function lidar_step(dt)
    local step = 360.0 / P.lidar_points
    local adv = 360.0 * P.lidar_hz * dt
    local a = S.lidar_angle
    local stop = a + adv
    local B, nb = lidar_boxes_now()
    local ox, oy = S.x + math.cos(S.th) * S.lidar_dx, S.y + math.sin(S.th) * S.lidar_dx
    while a < stop do
        local raw = a % 360.0
        -- raw angle: clockwise from the LiDAR's front mark; the mark faces
        -- lidar_front_raw degrees from the robot's front
        local phi = math.rad(raw - P.lidar_front_raw)
        local d = lidar_ray(phi, ox, oy, B, nb)
        local mm = 0.0
        if d and d >= 0.02 and d < P.lidar_max then
            mm = d * 1000.0
            mm = mm + gauss() * (P.lidar_sigma_mm + P.lidar_sigma_rel * mm)
        end
        table.insert(S.lidar_points, raw)
        table.insert(S.lidar_points, math.max(0, mm))
        local nxt = a + step * (0.9 + 0.2 * math.random())
        if math.floor(nxt / 360.0) > math.floor(a / 360.0) then
            -- one revolution complete
            S.lidar_scan = sim.packFloatTable(S.lidar_points)
            S.lidar_n = #S.lidar_points // 2
            S.lidar_points = {}
            S.lidar_seq = S.lidar_seq + 1
        end
        a = nxt
    end
    S.lidar_angle = a - 360.0 * math.floor(a / 360.0)
end

-- Time spent in each part of the step, for tools/profile_step.py.
PROF = {n = 0, serve = 0, drive = 0, kin = 0, imu = 0, referee = 0, lidar = 0, cam = 0, cam_n = 0}
local function timed(key, f, ...)
    local t = sim.getSystemTime()
    local r = f(...)
    PROF[key] = PROF[key] + (sim.getSystemTime() - t)
    return r
end

function profile_get(reset)
    local n = math.max(1, PROF.n)
    local out = string.format(
        'steps %d | per step, ms: serve %.2f (of it camera %.2f, %d frames) drive+servo %.2f kinematics %.2f imu %.2f referee %.2f lidar %.2f',
        PROF.n, PROF.serve / n * 1000, PROF.cam / n * 1000, PROF.cam_n, PROF.drive / n * 1000, PROF.kin / n * 1000,
        PROF.imu / n * 1000, PROF.referee / n * 1000, PROF.lidar / n * 1000)
    if reset then
        for k, _ in pairs(PROF) do PROF[k] = 0 end
    end
    return out
end

function sysCall_actuation()
    if S.replay then return end
    PROF.n = PROF.n + 1
    timed('serve', serve)
    local dt = sim.getSimulationTimeStep()
    local t = sim.getSimulationTime()
    local v = timed('drive', function() servo_step(dt) return drive_step(dt) end)
    timed('kin', kinematics_step, dt, v)
    timed('imu', imu_step, dt, t)
    timed('referee', referee_step, t, v)
end

function sysCall_sensing()
    if S.replay then return end
    timed('lidar', lidar_step, sim.getSimulationTimeStep())
    timed('serve', serve)
end

-- ------------------------------------------------------------------ referee
function log_event(text)
    local t = S.run_t0 and (sim.getSimulationTime() - S.run_t0) or 0
    local line = string.format('[%6.1f s] %s', t, text)
    sim.addLog(sim.verbosity_scriptinfos, 'WRO twin ' .. line)
    S.events = S.events or {}
    table.insert(S.events, line)
end

function press_button()
    S.button_until = sim.getSimulationTime() + 0.4
    if not S.run_t0 then
        S.run_t0 = sim.getSimulationTime()
        S.run_wall0 = sim.getSystemTime()
        S.progress = 0
        S.last_polar = math.atan2(S.y, S.x)
        log_event('start button pressed')
    end
end

local function car_corners()
    -- outline of the car seen from above (FIELD.footprint, from the CAD)
    local pts = {}
    for _, q in ipairs(FIELD.footprint or {{-0.040, 0.0785}, {0.2035, 0.0785}, {0.2035, -0.0785}, {-0.040, -0.0785}}) do
        local c, s = math.cos(S.th), math.sin(S.th)
        table.insert(pts, {S.x + c * q[1] - s * q[2], S.y + s * q[1] + c * q[2]})
    end
    return pts
end

function parking_verdict()
    local L = FIELD.lot
    local inside = true
    for _, p in ipairs(car_corners()) do
        if p[1] < L.x0 or p[1] > L.x1 or p[2] < L.y0 or p[2] > L.y1 then inside = false end
    end
    -- parallel: both driving (rear) wheels within 2 cm of the same wall distance
    local wall_dir = L.wall_dir or math.pi / 2
    local ang = math.abs(wrap180(math.deg(S.th - wall_dir)))
    ang = math.min(ang, 180 - ang)
    local diff = math.abs(math.sin(math.rad(ang))) * P.wheelbase  -- spread across the car
    local parallel = diff <= 0.02
    if inside and parallel then return 'FULLY PARKED' end
    if inside then return 'PARKED, NOT PARALLEL' end
    local any = false
    for _, p in ipairs(car_corners()) do
        if p[1] >= L.x0 and p[1] <= L.x1 and p[2] >= L.y0 and p[2] <= L.y1 then any = true end
    end
    return any and 'PARTLY PARKED' or 'NOT PARKED'
end

-- side on which the car passes each sign: the moment a sign goes from in
-- front of the car to behind it while the car drives forward. Passing on the
-- right = the sign stays on the car's left. Modified signs (surprise rule) are
-- judged by their original colour.
function passages_step(v)
    if not S.run_t0 or v <= 0.02 then return end
    local cx = S.x + 0.08 * math.cos(S.th)
    local cy = S.y + 0.08 * math.sin(S.th)
    local c, sn = math.cos(S.th), math.sin(S.th)
    local lap = math.floor(math.abs(S.progress) / (2 * math.pi)) + 1
    for _, sg in ipairs(S.signs) do
        local p = sim.getObjectPosition(sg.h, sim.handle_world)
        local dx, dy = p[1] - cx, p[2] - cy
        local f = dx * c + dy * sn
        local l = -dx * sn + dy * c
        if sg.f and sg.f > 0 and f <= 0 and math.abs(l) < 0.45 and lap <= 3 then
            sg.passed = sg.passed or {}
            if not sg.passed[lap] then
                local info = (FIELD.signs or {})[sg.alias] or {}
                local original = info.original or (sg.alias:find('red') and 'red' or 'green')
                local side = (l > 0) and 'right' or 'left'
                local want = (original == 'red') and 'right' or 'left'
                sg.passed[lap] = (side == want)
                local mod = info.height and info.height ~= 100
                local text = string.format('lap %d: %s%s passed on the %s %s', lap, sg.alias,
                    mod and (' (original ' .. original .. ')') or '', side, (side == want) and 'OK' or 'WRONG')
                log_event(text)
                S.last_passage = text
                if mod then
                    S.surprise_ok = (S.surprise_ok or 0) + ((side == want) and 1 or 0)
                    S.surprise_n = (S.surprise_n or 0) + 1
                end
            end
        end
        sg.f = f
    end
end

-- Record of the run for tools/replay_video.py: the video is rendered after
-- the run from this record, so recording does not slow the simulation down.
function replay_step(t, v)
    if S.rec_t and t - S.rec_t < 0.04 then return end
    S.rec_t = t
    S.rec = S.rec or {}
    local touched = 0
    for _, s in ipairs(S.signs) do if s.touched then touched = touched + 1 end end
    local r3 = function(x) return math.floor(x * 1000 + 0.5) / 1000 end
    table.insert(S.rec, {r3(t), r3(S.x), r3(S.y), r3(S.th), r3(math.deg(steer_angle())), r3(S.wheel_angle),
        r3(v), S.run_t0 and r3(t - S.run_t0) or -1, r3(math.abs(S.progress) / (2 * math.pi)),
        touched, S.wall_hits, S.surprise_ok or 0, S.surprise_n or 0, S.led_on and 1 or 0})
    S.rec_signs = S.rec_signs or {}
    for i, sg in ipairs(S.signs) do
        local p = sim.getObjectPosition(sg.h, sim.handle_world)
        if not sg.rec or math.abs(p[1] - sg.rec[1]) + math.abs(p[2] - sg.rec[2]) > 0.002 then
            sg.rec = {p[1], p[2]}
            table.insert(S.rec_signs, {r3(t), sg.alias, r3(p[1]), r3(p[2])})
        end
    end
end

-- Replay mode (tools/replay_video.py): the car is placed from a record and
-- the board, the sensors and the referee are switched off.
function replay_pose(x, y, th, steer_deg, wheel_angle)
    S.replay = true
    S.x, S.y, S.th = x, y, th
    sim.setObjectPosition(S.car, {x, y, 0}, sim.handle_world)
    sim.setObjectOrientation(S.car, {0, 0, th}, sim.handle_world)
    for _, h in pairs(S.wheels) do sim.setObjectOrientation(h, {0, wheel_angle, 0}, sim.handle_parent) end
    local delta, tl, tr = math.rad(steer_deg), 0, 0
    if math.abs(delta) > 1e-4 then
        local R = P.wheelbase / math.tan(delta)
        tl = math.atan(P.wheelbase / (R - P.half_track))
        tr = math.atan(P.wheelbase / (R + P.half_track))
    end
    sim.setObjectOrientation(S.steer_l, {0, 0, tl}, sim.handle_parent)
    sim.setObjectOrientation(S.steer_r, {0, 0, tr}, sim.handle_parent)
end

-- One video frame of the replay in a single call: car pose, moved signs,
-- chase camera, both views rendered. Returns the two RGB images.
function replay_render(x, y, th, steer_deg, wheel_angle, signs)
    replay_pose(x, y, th, steer_deg, wheel_angle)
    for _, sg in ipairs(signs or {}) do
        sim.setObjectPosition(sg[1], {sg[2], sg[3], sg[4]}, sim.handle_world)
    end
    S.rec_overview = S.rec_overview or sim.getObject('/recOverview')
    S.rec_chase = S.rec_chase or sim.getObject('/recChase')
    local c, s = math.cos(th), math.sin(th)
    local eye = {x - 0.55 * c, y - 0.55 * s, 0.42}
    local z = {0.80 * c, 0.80 * s, -0.39}
    local n = math.sqrt(z[1] ^ 2 + z[2] ^ 2 + z[3] ^ 2)
    z = {z[1] / n, z[2] / n, z[3] / n}
    local up = {-z[3] * z[1], -z[3] * z[2], 1 - z[3] * z[3]}
    n = math.sqrt(up[1] ^ 2 + up[2] ^ 2 + up[3] ^ 2)
    local yv = {up[1] / n, up[2] / n, up[3] / n}
    local xv = {yv[2] * z[3] - yv[3] * z[2], yv[3] * z[1] - yv[1] * z[3], yv[1] * z[2] - yv[2] * z[1]}
    sim.setObjectMatrix(S.rec_chase, {xv[1], yv[1], z[1], eye[1], xv[2], yv[2], z[2], eye[2],
                                      xv[3], yv[3], z[3], eye[3]}, sim.handle_world)
    sim.handleVisionSensor(S.rec_overview)
    sim.handleVisionSensor(S.rec_chase)
    local a = sim.getVisionSensorImg(S.rec_overview)
    local b = sim.getVisionSensorImg(S.rec_chase)
    return a, b
end

function referee_step(t, v)
    replay_step(t, v)
    passages_step(v)
    if S.trace_t < 0 or t - S.trace_t >= 0.1 then
        table.insert(S.trace, {math.floor(t * 100) / 100, math.floor(S.x * 1000), math.floor(S.y * 1000)})
        S.trace_t = t
    end
    if not S.run_t0 then
        update_ui(t)
        return
    end
    local a = math.atan2(S.y, S.x)
    local d = a - S.last_polar
    if d > math.pi then d = d - 2 * math.pi elseif d < -math.pi then d = d + 2 * math.pi end
    S.progress = S.progress + d
    S.last_polar = a
    local laps = math.abs(S.progress) / (2 * math.pi)
    if math.abs(v) < 0.01 and math.abs(S.rps) < 0.05 then
        S.still_since = S.still_since or t
    else
        S.still_since = nil
    end
    if laps > 2.75 and S.still_since and t - S.still_since > 3.0 and S.result == '' then
        S.result = parking_verdict()
        S.run_time = t - S.run_t0
        S.result_t = t
        log_event(string.format('stopped after %.2f laps: %s', laps, S.result))
    elseif S.result ~= '' and not S.still_since then
        S.result = ''   -- moved again: the manoeuvre was not finished
    end
    update_ui(t)
end

local function json(v)
    local t = type(v)
    if t == 'table' then
        if #v > 0 or next(v) == nil then
            local out = {}
            for _, x in ipairs(v) do table.insert(out, json(x)) end
            return '[' .. table.concat(out, ',') .. ']'
        end
        local out = {}
        for k, x in pairs(v) do table.insert(out, string.format('%q', tostring(k)) .. ':' .. json(x)) end
        return '{' .. table.concat(out, ',') .. '}'
    elseif t == 'string' then
        return string.format('%q', v)
    elseif t == 'number' then
        return tostring(v)
    elseif t == 'boolean' then
        return v and 'true' or 'false'
    end
    return 'null'
end

function save_result()
    if not S.run_t0 or S.replay then return end
    local dir = sim.getStringParam(sim.stringparam_scene_path) .. '/results'
    pcall(function()
        local name = dir .. '/run_' .. os.date('%Y%m%d_%H%M%S') .. '.json'
        local f = io.open(name, 'w')
        if not f then return end
        local touched = {}
        for _, s in ipairs(S.signs) do
            if s.touched then table.insert(touched, {sign = s.alias, moved_mm = math.floor(s.moved * 1000)}) end
        end
        local laps = math.abs(S.progress) / (2 * math.pi)
        f:write(json({
            layout = FIELD.name, direction = (S.progress < 0) and 'clockwise' or 'counter-clockwise',
            laps = math.floor(laps * 100) / 100, time_s = S.run_time, result = S.result,
            -- simulated time / real time while the program was running (1 = real time)
            sim_speed = (S.run_wall0 and S.req_wall and S.req_wall > S.run_wall0)
                and math.floor((S.req_sim - S.run_t0) / (S.req_wall - S.run_wall0) * 1000) / 1000 or 0,
            signs_touched = touched, wall_contacts = S.wall_hits, events = S.events or {},
            surprise_correct = S.surprise_ok or 0, surprise_passages = S.surprise_n or 0,
            scene = sim.getStringParam(sim.stringparam_scene_path_and_name), run_t0 = S.run_t0,
            replay_columns = {'t', 'x', 'y', 'th', 'steer_deg', 'wheel_angle', 'speed', 't_run', 'laps', 'touched',
                              'walls', 'surprise_ok', 'surprise_n', 'led'},
            replay = S.rec or {}, replay_signs = S.rec_signs or {},
            result_time = S.result_t,
            trace_t_x_y_mm = S.trace,
        }))
        f:close()
    end)
end

-- state for the video recorder (tools/record_run.py)
function hud()
    local touched = 0
    for _, s in ipairs(S.signs) do if s.touched then touched = touched + 1 end end
    return {
        started = S.run_t0 ~= nil,
        time = S.run_t0 and (sim.getSimulationTime() - S.run_t0) or 0,
        laps = math.abs(S.progress) / (2 * math.pi),
        direction = (S.progress < 0) and 'clockwise' or ((S.progress > 0) and 'counter-clockwise' or ''),
        touched = touched, walls = S.wall_hits, result = S.result,
        steer = math.deg(steer_angle()), speed = S.rps * math.pi * P.enc_wheel_diam,
        x = S.x, y = S.y, th = S.th, led = S.led_on, layout = FIELD.name,
        last_passage = S.last_passage or '', surprise_ok = S.surprise_ok or 0, surprise_n = S.surprise_n or 0,
    }
end

-- ------------------------------------------------------------------ UI
function build_ui()
    local xml = [[
<ui title="WRO twin - The Maker v7" closeable="false" resizable="false" placement="relative" position="20,20" layout="vbox">
  <label id="1" text="Obstacle Challenge" style="font-weight: bold; font-size: 14px;" />
  <button id="2" text="START button (GPIO4)" on-click="on_start" />
  <label id="3" text="Waiting for the program..." />
  <label id="4" text="" />
  <label id="5" text="" />
  <label id="6" text="" style="font-weight: bold;" />
</ui>]]
    S.ui = simUI.create(xml)
end

function on_start(ui, id)
    press_button()
end

function update_ui(t)
    if not S.ui then return end
    S.ui_t = S.ui_t or -1
    if t - S.ui_t < 0.2 then return end
    S.ui_t = t
    local status
    if not S.run_t0 then
        status = S.led_on and 'Program ready: LED blinking - press START' or 'Waiting for the program...'
    else
        status = string.format('Time %.1f s   laps %.2f   %s', t - S.run_t0, math.abs(S.progress) / (2 * math.pi),
            (S.progress < 0) and 'clockwise' or ((S.progress > 0) and 'counter-clockwise' or ''))
    end
    simUI.setLabelText(S.ui, 3, status)
    local touched = 0
    for _, s in ipairs(S.signs) do if s.touched then touched = touched + 1 end end
    simUI.setLabelText(S.ui, 4, string.format('Signs touched: %d   wall contacts: %d', touched, S.wall_hits))
    simUI.setLabelText(S.ui, 5, string.format('Steering %+.1f deg   speed %.2f m/s', math.deg(steer_angle()),
        S.rps * math.pi * P.enc_wheel_diam))
    simUI.setLabelText(S.ui, 6, S.result)
end
