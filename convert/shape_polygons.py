import math

POLYGONS = {}

triangle_offset = -0
triangle_width = 0.29
triangle_height = triangle_width / 2 * math.tan(60 * math.pi / 180)
triangle_center = triangle_width * math.sqrt(3) / 3
POLYGONS["triangle"] = [
    [-(triangle_center), 0, 0],
    [triangle_height - triangle_center, 0, -triangle_width / 2],
    [triangle_height - triangle_center, 0, triangle_width / 2],
    [-(triangle_center), 0, 0],
]
circle_radius = 0.292 / 2
iterations = 150
circle_polygon = []
for i in range(iterations):
    circle_polygon.append(
        [
            math.sin(2 * math.pi * i / iterations) * circle_radius,
            0,
            math.cos(2 * math.pi * i / iterations) * circle_radius,
        ]
    )
POLYGONS["circle"] = circle_polygon

square_width = 0.29
POLYGONS["square"] = [
    [-square_width / 2.0, 0, -square_width / 2.0],
    [square_width / 2.0, 0, -square_width / 2.0],
    [square_width / 2.0, 0, square_width / 2.0],
    [-square_width / 2.0, 0, square_width / 2.0],
    [-square_width / 2.0, 0, -square_width / 2.0],
]

offset_x = 0.28 / 2 - 0.022154 - 0.05
offset_z = 0.22 / 2 + 0.1 + 0.05
POLYGONS["blob"] = [
    [offset_x, 0, offset_z],
    [offset_x + 0.022154, 0, offset_z],
    [offset_x + 0.022154, 0, offset_z - 0.1],
    [offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1],
    [offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1 - 0.22],
    [offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22],
    [offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22 + 0.25],
    [offset_x, 0, offset_z],
]

blade_length = 0.29
saw_handle_thickness = 0.01
offset_x = -0.18
offset_z = -0
POLYGONS["saw"] = [
    [offset_x, 0, offset_z],
    [offset_x, 0, offset_z - 0.03],
    [offset_x + blade_length, 0, offset_z - 0.03],
    [offset_x + blade_length, 0, offset_z - 0.03],
    [offset_x + blade_length, 0, offset_z - 0.03 + 0.097],
    [offset_x, 0, offset_z],
    [offset_x + blade_length, saw_handle_thickness, offset_z - 0.03],
    [offset_x + blade_length + 0.085, saw_handle_thickness, offset_z - 0.03],
    [
        offset_x + blade_length + 0.085 + 0.05,
        saw_handle_thickness,
        offset_z - 0.03 + 0.085,
    ],
    [offset_x + blade_length + 0.043, saw_handle_thickness, offset_z - 0.03 + 0.105],
    [offset_x + blade_length, -saw_handle_thickness, offset_z - 0.03],
    [offset_x + blade_length + 0.085, -saw_handle_thickness, offset_z - 0.03],
    [
        offset_x + blade_length + 0.085 + 0.05,
        -saw_handle_thickness,
        offset_z - 0.03 + 0.085,
    ],
    [offset_x + blade_length + 0.043, -saw_handle_thickness, offset_z - 0.03 + 0.105],
]

offset_x = -0
offset_z = 0.014
hammer_total_width = 0.1
hammer_thinnering = 0.002
hammer_dip_right = 0.03
hammer_front_length = 0.02
hammer_back_length = 0.05
hammer_front_thickness = 0.024
hammer_stem_thickness = 0.013
hammer_stem_length = 0.097
hammer_spike_thing_down = 0.011

POLYGONS["hammer"] = [
    [offset_x, 0, offset_z],
    [offset_x + hammer_dip_right, 0, offset_z],
    [offset_x + hammer_dip_right, 0, offset_z - hammer_thinnering],
    [
        offset_x + hammer_dip_right + hammer_front_length,
        0,
        offset_z - hammer_thinnering,
    ],
    [
        offset_x + hammer_dip_right + hammer_front_length,
        0,
        offset_z - hammer_thinnering - hammer_front_thickness,
    ],
    [
        offset_x + hammer_stem_thickness / 2,
        0,
        offset_z - hammer_thinnering - hammer_front_thickness,
    ],
    [
        offset_x + hammer_stem_thickness / 2,
        0,
        offset_z - hammer_thinnering - hammer_front_thickness - hammer_stem_length,
    ],
    [
        offset_x - hammer_stem_thickness / 2,
        0,
        offset_z - hammer_thinnering - hammer_front_thickness - hammer_stem_length,
    ],
    [
        offset_x - hammer_stem_thickness / 2,
        0,
        offset_z - hammer_thinnering - hammer_front_thickness,
    ],
    [
        offset_x - hammer_stem_thickness / 2 - hammer_back_length,
        0,
        offset_z - hammer_thinnering - hammer_front_thickness - hammer_spike_thing_down,
    ],
    [offset_x, 0, offset_z],
]

plier_top_edge_to_clip_top = 0.018
plier_top_width = 0.055
plier_bot_width = 0.110
plier_length = 0.083
offset_x = 0
offset_z = -plier_top_edge_to_clip_top

lower_layer = -0.008
upper_layer = 0.007
POLYGONS["pliers"] = [
    [-23 / 1000.0, lower_layer, 35 / 1000.0],
    [23 / 1000.0, lower_layer, 35 / 1000.0],
    [-55 / 1000.0, lower_layer, -101 / 1000.0],
    [55 / 1000.0, lower_layer, -101 / 1000.0],
    [-23 / 1000.0, upper_layer, 35 / 1000.0],
    [23 / 1000.0, upper_layer, 35 / 1000.0],
    [-55 / 1000.0, upper_layer, -101 / 1000.0],
    [55 / 1000.0, upper_layer, -101 / 1000.0],
]

screwdriver_thickness = 0.009
screwdriver_length = 0.2
offset_x = -0.0045
offset_z = 0.1
POLYGONS["screwdriver"] = [
    [offset_x, 0, offset_z],
    [offset_x + screwdriver_thickness, 0, offset_z],
    [offset_x + screwdriver_thickness, 0, offset_z - screwdriver_length],
    [offset_x, 0, offset_z - screwdriver_length],
    [offset_x, 0, offset_z],
]


handle_length = 130 / 1000.0
left_top_length = 45 / 1000.0
right_top_length = 53 / 1000.0
top_bulb_width = 16 / 1000.0
indent = 5 / 1000.0
handle_to_left = 26 / 1000.0
handle_to_right = 30 / 1000.0
handle_width = 20 / 1000.0
offset_x = handle_width / 2
offset_z = -handle_length
POLYGONS["wrench"] = [
    [offset_x, 0, offset_z],
    [offset_x, 0, offset_z + handle_length],
    [offset_x + top_bulb_width, 0, offset_z + handle_length + left_top_length / 2],
    [offset_x + indent, 0, offset_z + handle_length + left_top_length],
    [offset_x, 0, offset_z + handle_length + handle_to_left],
    [offset_x - handle_width, 0, offset_z + handle_length + handle_to_right],
    [offset_x - handle_width + indent, 0, offset_z + handle_length + right_top_length],
    [
        offset_x - handle_width - top_bulb_width,
        0,
        offset_z + handle_length + right_top_length / 2,
    ],
    [offset_x - handle_width, 0, offset_z + handle_length],
    [offset_x - handle_width, 0, offset_z],
    [offset_x, 0, offset_z],
]
