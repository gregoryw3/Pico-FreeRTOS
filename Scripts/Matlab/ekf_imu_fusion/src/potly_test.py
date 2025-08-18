import numpy as np
import pandas as pd
import plotly.graph_objects as go
from scipy.spatial.transform import Rotation as R

# Load your data as before
df = pd.read_csv("./BlRv_SN0867 HR_06-11-2025_12_07_53.csv")
quats = df[['Quat_1', 'Quat_2', 'Quat_3', 'Quat_4']].values  # [w, x, y, z]
times = df['Flight_Time_(s)'].values
N = len(quats)
scale = 10

df_lr = pd.read_csv("./BlRv_SN0867 LR_06-11-2025_12_07_53.csv")
times_lr = df_lr['Flight_Time_(s)'].values
alt_lr = df_lr['Baro_Altitude_AGL_(feet)'].values

# Helper to create axis lines for a given quaternion
def get_axes_lines(q, scale=10):
    rot = R.from_quat([q[1], q[2], q[3], q[0]])  # [x, y, z, w]
    Rmat = R.from_euler('y', 270, degrees=True).as_matrix() @ rot.as_matrix()
    origin = np.zeros(3)
    lines = []
    colors = ['red', 'green', 'blue']
    for i in range(3):
        lines.append(go.Scatter3d(
            x=[origin[0], Rmat[0, i]*scale],
            y=[origin[1], Rmat[1, i]*scale],
            z=[origin[2], Rmat[2, i]*scale],
            mode='lines',
            line=dict(color=colors[i], width=8),
            showlegend=False
        ))
    return lines

# Initial frame
frame0 = get_axes_lines(quats[0], scale=scale)

# Altitude trace
alt_trace = go.Scatter(
    x=times_lr,
    y=alt_lr,
    mode='lines',
    name='Altitude'
)
alt_marker = go.Scatter(
    x=[times[0]],
    y=[np.interp(times[0], times_lr, alt_lr)],
    mode='markers',
    marker=dict(color='red', size=10),
    name='Current'
)

# Build frames for animation
frames = []
for i in range(N):
    axes = get_axes_lines(quats[i], scale=scale)
    marker = go.Scatter(
        x=[times[i]],
        y=[np.interp(times[i], times_lr, alt_lr)],
        mode='markers',
        marker=dict(color='red', size=10),
        showlegend=False
    )
    frames.append(go.Frame(
        data=axes + [alt_trace, marker],
        name=str(i)
    ))

# Layout with play/pause
layout = go.Layout(
    scene=dict(
        xaxis=dict(range=[-scale, scale]),
        yaxis=dict(range=[-scale, scale]),
        zaxis=dict(range=[-scale, scale]),
        aspectmode='cube',
        title='Orientation'
    ),
    xaxis=dict(title='Time (s)'),
    yaxis=dict(title='Altitude (m)'),
    updatemenus=[dict(
        type='buttons',
        showactive=False,
        y=1,
        x=1.1,
        xanchor='right',
        yanchor='top',
        buttons=[
            dict(label='Play', method='animate', args=[None, {"frame": {"duration": 50, "redraw": True}, "fromcurrent": True, "mode": "immediate"}]),
            dict(label='Pause', method='animate', args=[[None], {"frame": {"duration": 0, "redraw": False}, "mode": "immediate"}])
        ]
    )]
)

# Combine everything
fig = go.Figure(
    data=frame0 + [alt_trace, alt_marker],
    layout=layout,
    frames=frames
)

fig.show()