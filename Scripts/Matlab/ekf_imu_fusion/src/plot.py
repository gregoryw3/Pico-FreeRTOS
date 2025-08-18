import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider
from scipy.spatial.transform import Rotation as R
from matplotlib.widgets import Button
import time

# Load high-rate data
df = pd.read_csv("./BlRv_SN0867 HR_06-11-2025_12_07_53.csv")
quats = df[['Quat_1', 'Quat_2', 'Quat_3', 'Quat_4']].values  # [w, x, y, z]
times = df['Flight_Time_(s)'].values
N = len(quats)
scale = 10

# Load low-rate data
df_lr = pd.read_csv("./BlRv_SN0867 LR_06-11-2025_12_07_53.csv")
times_lr = df_lr['Flight_Time_(s)'].values
alt_lr = df_lr['Baro_Altitude_AGL_(feet)'].values # convert to meters

fig = plt.figure(figsize=(10, 8))
ax_orient = fig.add_subplot(211, projection='3d')
ax_alt = fig.add_subplot(212)
plt.subplots_adjust(bottom=0.15, hspace=0.35)

# Plot altitude curve
ax_alt.plot(times_lr, alt_lr, 'b-')
alt_marker, = ax_alt.plot([times[0]], [np.interp(times[0], times_lr, alt_lr)], 'ro')
ax_alt.set_xlabel('Time (s)')
ax_alt.set_ylabel('Altitude (m)')
ax_alt.set_title('Barometric Altitude')
ax_alt.grid(True)

def plot_orientation(idx):
    ax_orient.cla()
    rot = R.from_quat([quats[idx,1], quats[idx,2], quats[idx,3], quats[idx,0]])  # [x, y, z, w]
    Rmat = rot.as_matrix()

    # Rotate coordinate system for visualization
    Rmat = R.from_euler('y', 270, degrees=True).as_matrix() @ Rmat
    
    # Draw orientation axes
    origin = np.zeros(3)

    ax_orient.quiver(*origin, *Rmat[:,0]*scale, color='r')
    ax_orient.quiver(*origin, *Rmat[:,1]*scale, color='g')
    ax_orient.quiver(*origin, *Rmat[:,2]*scale, color='b')

    ax_orient.set_xlim([-scale, scale])
    ax_orient.set_ylim([-scale, scale])
    ax_orient.set_zlim([-scale, scale])
    ax_orient.set_title(f'Time: {times[idx]:.2f}s')
    ax_orient.set_xlabel('X')
    ax_orient.set_ylabel('Y')
    ax_orient.set_zlabel('Z')
    # Update altitude marker
    cur_time = times[idx]
    cur_alt = np.interp(cur_time, times_lr, alt_lr)
    alt_marker.set_data([cur_time], [cur_alt])
    fig.canvas.draw_idle()

# Slider setup
ax_slider = plt.axes([0.15, 0.05, 0.7, 0.03])
slider = Slider(ax_slider, 'Frame', 0, N-1, valinit=0, valstep=1)

def update(val):
    idx = int(slider.val)
    plot_orientation(idx)

slider.on_changed(update)

# Add buttons for play, pause, and speed
ax_play = plt.axes([0.15, 0.01, 0.1, 0.04])
ax_pause = plt.axes([0.27, 0.01, 0.1, 0.04])
ax_speed = plt.axes([0.39, 0.01, 0.1, 0.04])
btn_play = Button(ax_play, 'Play')
btn_pause = Button(ax_pause, 'Pause')
btn_speed = Button(ax_speed, 'Speed x1')

playing = [False]
speed = [1]

start_wall_time = [None]
start_data_time = [None]

def play(event):
    playing[0] = True
    start_wall_time[0] = time.time()
    start_data_time[0] = times[int(slider.val)]

def pause(event):
    playing[0] = False

def speedup(event):
    speed[0] = 2 if speed[0] == 1 else 1
    btn_speed.label.set_text(f"Speed x{speed[0]}")

btn_play.on_clicked(play)
btn_pause.on_clicked(pause)
btn_speed.on_clicked(speedup)

def animate(event):
    if playing[0]:
        elapsed = (time.time() - start_wall_time[0]) * speed[0]
        cur_data_time = start_data_time[0] + elapsed
        idx = np.searchsorted(times, cur_data_time)
        if idx >= N:
            idx = 0
            playing[0] = False  # stop at end
        slider.set_val(idx)
    fig.canvas.draw_idle()

timer = fig.canvas.new_timer(interval=20)  # 20 ms for smoothness
timer.add_callback(animate, None)
timer.start()

# Initial plot
plot_orientation(0)
plt.show()