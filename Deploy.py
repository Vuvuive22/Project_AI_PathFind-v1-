from flask import Flask, render_template, request, jsonify, url_for, redirect, session
import osmnx as ox
import networkx as nx
import os
import shutil
from shortest_path import *
from geopy.distance import geodesic


app = Flask(__name__)
app.secret_key = 'your_secret_key'
banned_edges = []
banned_areas = []
tac_edges = []
flooded_edges = []

congvi_map = ox.load_graphml('congvi_badinh_hanoi_graph2.graphml')
G = Create_simple_Graph(congvi_map)

@app.route('/')
def index():
    node_coords = [(congvi_map.nodes[node]['y'], congvi_map.nodes[node]['x']) for node in congvi_map.nodes]
    path_coords = [
        [(congvi_map.nodes[e[0]]['y'], congvi_map.nodes[e[0]]['x']), (congvi_map.nodes[e[1]]['y'], congvi_map.nodes[e[1]]['x'])]
        for e in congvi_map.edges
    ]
    return render_template('index.html', node_coords=node_coords, path_coords=path_coords)

@app.route('/admin', methods=['GET', 'POST'])
def admin_login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        if username == 'admin' and password == 'admin123':
            session['admin_logged_in'] = True
            return redirect(url_for('admin_panel'))
        else:
            return render_template('admin_login.html', error='Sai tài khoản hoặc mật khẩu!')
    return render_template('admin_login.html')

@app.route('/admin/panel')
def admin_panel():
    if not session.get('admin_logged_in'):
        return redirect(url_for('admin_login'))
    node_coords = [(congvi_map.nodes[node]['y'], congvi_map.nodes[node]['x']) for node in congvi_map.nodes]
    path_coords = [
        [(congvi_map.nodes[e[0]]['y'], congvi_map.nodes[e[0]]['x']), (congvi_map.nodes[e[1]]['y'], congvi_map.nodes[e[1]]['x'])]
        for e in congvi_map.edges
    ]
    return render_template('admin_panel.html', node_coords=node_coords, path_coords=path_coords)

@app.route('/admin/logout')
def admin_logout():
    session.pop('admin_logged_in', None)
    return redirect(url_for('admin_login'))

# Cấm đường (admin gọi ajax)
@app.route('/ban_edge', methods=['POST'])
def ban_edge():
    if not session.get('admin_logged_in'):
        return jsonify({"error": "Unauthorized"}), 401
    data = request.json
    node1_coords = data['node1']
    node2_coords = data['node2']
    direction = data.get('direction', 'both')
    node1 = ox.distance.nearest_nodes(congvi_map, node1_coords[1], node1_coords[0])
    node2 = ox.distance.nearest_nodes(congvi_map, node2_coords[1], node2_coords[0])
    if not congvi_map.has_edge(node1, node2) and not congvi_map.has_edge(node2, node1):
        return jsonify({"error": "Edge not found"}), 404
    if direction == 'both':
        if congvi_map.has_edge(node1, node2):
            congvi_map.remove_edge(node1, node2)
        if congvi_map.has_edge(node2, node1):
            congvi_map.remove_edge(node2, node1)
    elif direction == 'one-way':
        if congvi_map.has_edge(node1, node2):
            congvi_map.remove_edge(node1, node2)
    # Lưu lại đoạn vừa cấm
    banned_edges.append({'node1': node1, 'node2': node2, 'direction': direction})
    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Đã cấm đường thành công!"})

@app.route('/ban_area', methods=['POST'])
def ban_area():
    if not session.get('admin_logged_in'):
        return jsonify({"error": "Unauthorized"}), 401
    data = request.json
    polygon = data.get('polygon')  # Danh sách [lat, lon]
    if not polygon or len(polygon) < 3:
        return jsonify({"error": "Vùng không hợp lệ"}), 400
    banned_areas.append({'polygon': polygon})
    # Xóa các cạnh nằm trong vùng cấm
    nodes_in_area = []
    for node in congvi_map.nodes:
        y, x = congvi_map.nodes[node]['y'], congvi_map.nodes[node]['x']
        if point_in_polygon((y, x), polygon):
            nodes_in_area.append(node)
    removed_edges = []
    for u, v, k in list(congvi_map.edges(keys=True)):
        if u in nodes_in_area or v in nodes_in_area:
            congvi_map.remove_edge(u, v, key=k)
            removed_edges.append((u, v, k))
    banned_areas[-1]['removed_edges'] = removed_edges  # Lưu lại để khôi phục
    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Đã cấm vùng thành công!"})

@app.route('/restore_last_ban', methods=['POST'])
def restore_last_ban():
    if not session.get('admin_logged_in'):
        return jsonify({"error": "Unauthorized"}), 401
    if not banned_edges:
        return jsonify({"error": "Không có đoạn nào để khôi phục!"}), 400
    last = banned_edges.pop()
    node1 = last['node1']
    node2 = last['node2']
    direction = last['direction']
    # Thêm lại edge vào graph
    # Lấy thuộc tính gốc từ file graphml (nếu cần chính xác hơn)
    attrs = {'length': 1}
    if direction == 'both':
        congvi_map.add_edge(node1, node2, **attrs)
        congvi_map.add_edge(node2, node1, **attrs)
    elif direction == 'one-way':
        congvi_map.add_edge(node1, node2, **attrs)
    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Đã khôi phục đoạn vừa cấm!"})

@app.route('/restore_last_ban_area', methods=['POST'])
def restore_last_ban_area():
    if not session.get('admin_logged_in'):
        return jsonify({"error": "Unauthorized"}), 401
    if not banned_areas:
        return jsonify({"error": "Không có vùng nào để khôi phục!"}), 400
    last = banned_areas.pop()
    for u, v, k in last.get('removed_edges', []):
        congvi_map.add_edge(u, v, key=k, length=1)
    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Đã khôi phục vùng vừa cấm!"})

@app.route('/tac_edge', methods=['POST'])
def tac_edge():
    if not session.get('admin_logged_in'):
        return jsonify({"error": "Unauthorized"}), 401
    data = request.json
    node1_coords = data['node1']
    node2_coords = data['node2']
    factor = data.get('factor', 5)  # Hệ số độ tắc, mặc định gấp 5 lần
    node1 = ox.distance.nearest_nodes(congvi_map, node1_coords[1], node1_coords[0])
    node2 = ox.distance.nearest_nodes(congvi_map, node2_coords[1], node2_coords[0])
    if not congvi_map.has_edge(node1, node2):
        return jsonify({"error": "Edge not found"}), 404
    orig_lengths = []
    for key in congvi_map[node1][node2]:
        orig_len = congvi_map[node1][node2][key].get('length', 1)
        congvi_map[node1][node2][key]['length'] = orig_len * factor
        orig_lengths.append((key, orig_len))
    tac_edges.append({'node1': node1, 'node2': node2, 'orig_lengths': orig_lengths})
    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Đã đánh dấu tắc đường (tăng thời gian đi qua)!"})

@app.route('/restore_last_tac', methods=['POST'])
def restore_last_tac():
    if not session.get('admin_logged_in'):
        return jsonify({"error": "Unauthorized"}), 401
    if not tac_edges:
        return jsonify({"error": "Không có đoạn tắc nào để khôi phục!"}), 400
    last = tac_edges.pop()
    node1 = last['node1']
    node2 = last['node2']
    for key, orig_len in last['orig_lengths']:
        congvi_map[node1][node2][key]['length'] = orig_len
    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Đã khôi phục đoạn tắc đường!"})

@app.route('/flood_road', methods=['POST'])
def flood_road():
    print("Đã nhận request flood_road")  # Thêm dòng này
    data = request.json
    water_level = float(data['water_level'])
    flooded_area = data['flooded_area']  # List of [lat, lon]
    ban_threshold = 0.5  # m (ví dụ)
    slow_threshold = 0.2  # m (ví dụ)
    slow_factor = 5

    flooded_edges.clear()  # Clear previous flooded edges

    for node in list(congvi_map.nodes):
        y, x = congvi_map.nodes[node]['y'], congvi_map.nodes[node]['x']
        if point_in_polygon((y, x), flooded_area):
            for neighbor in list(congvi_map.neighbors(node)):
                for key in list(congvi_map[node][neighbor]):
                    edge = congvi_map[node][neighbor][key]
                    orig_length = edge.get('length', 1)
                    if water_level >= ban_threshold:
                        congvi_map.remove_edge(node, neighbor, key=key)
                        flooded_edges.append({'node1': node, 'node2': neighbor, 'key': key, 'type': 'ban'})
                    elif water_level >= slow_threshold:
                        edge['length'] = orig_length * slow_factor
                        flooded_edges.append({'node1': node, 'node2': neighbor, 'key': key, 'type': 'slow', 'orig_length': orig_length})

    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Flooded roads updated successfully!"})

@app.route('/restore_last_flood', methods=['POST'])
def restore_last_flood():
    if not session.get('admin_logged_in'):
        return jsonify({"error": "Unauthorized"}), 401
    if not flooded_edges:
        return jsonify({"error": "Không có vùng ngập nào để khôi phục!"}), 400

    last_flood = flooded_edges.copy()
    for edge in last_flood[::-1]:
        node1 = edge['node1']
        node2 = edge['node2']
        key = edge['key']
        if edge['type'] == 'ban':
            # Thêm lại cạnh đã xóa, có thể cần thuộc tính gốc, ở đây gán length=1
            congvi_map.add_edge(node1, node2, key=key, length=1)
        elif edge['type'] == 'slow':
            # Khôi phục lại độ dài gốc
            if congvi_map.has_edge(node1, node2, key=key):
                congvi_map[node1][node2][key]['length'] = edge['orig_length']
    flooded_edges.clear()
    global G
    G = Create_simple_Graph(congvi_map)
    return jsonify({"message": "Đã khôi phục vùng ngập gần nhất!"})

algorithm_list = {
    'Dijkstra': Dijkstra, 
    'A Star': A_star, 
    'UCS': UCS,
    'Greedy BFS': Greedy_best_first_search,
}

@app.route('/find_shortest_path', methods=['POST'])
def find_shortest_path():
    data = request.json
    start_coords = data['start']
    end_coords = data['end']
    algorithm = data['algorithm']
    max_depth = int(data['max_depth'])
    start_node = ox.distance.nearest_nodes(congvi_map, start_coords[1], start_coords[0])
    end_node = ox.distance.nearest_nodes(congvi_map, end_coords[1], end_coords[0])
    func = algorithm_list.get(algorithm)
    if not func:
        return jsonify({"error": "Invalid algorithm selected"}), 400
    path_coords = func(G, start_node, end_node)
    start_coords_path =[(congvi_map.nodes[start_node]['y'], congvi_map.nodes[start_node]['x']),(start_coords[0], start_coords[1])]
    end_coords_path =[(congvi_map.nodes[end_node]['y'], congvi_map.nodes[end_node]['x']),(end_coords[0], end_coords[1])]
    if path_coords is None:
        return jsonify({"error": "No path found"}), 404
    return jsonify({'path_coords': path_coords, 'max_depth': max_depth, 'start_path': start_coords_path , 'end_path': end_coords_path })

@app.route('/estimate_distance_time', methods=['POST'])
def estimate_distance_time():
    data = request.json
    start_coords = data['start']  # [lat, lon]
    end_coords = data['end']      # [lat, lon]
    speed = float(data.get('speed', 1.39))  # m/s, mặc định 5km/h

    # Tìm node gần nhất trên graph
    start_node = ox.distance.nearest_nodes(congvi_map, start_coords[1], start_coords[0])
    end_node = ox.distance.nearest_nodes(congvi_map, end_coords[1], end_coords[0])

    try:
        # Tìm đường đi ngắn nhất theo chiều dài thực tế
        path = nx.shortest_path(congvi_map, start_node, end_node, weight='length')
        # Tính tổng chiều dài các đoạn đường
        distance = sum(
            congvi_map.edges[path[i], path[i+1], 0].get('length', 0)
            for i in range(len(path)-1)
        )
    except Exception as e:
        return jsonify({'error': 'Không tìm được đường đi thực tế', 'detail': str(e)}), 400

    estimated_time = distance / speed if speed > 0 else None

    return jsonify({
        'distance_m': distance,
        'estimated_time_s': estimated_time
    })

def point_in_polygon(point, polygon):
    # point: (lat, lon), polygon: [(lat, lon), ...]
    x, y = point
    n = len(polygon)
    inside = False
    p1x, p1y = polygon[0]
    for i in range(n+1):
        p2x, p2y = polygon[i % n]
        if min(p1y, p2y) < y <= max(p1y, p2y):
            if x <= max(p1x, p2x):
                xinters = (y - p1y) * (p2x - p1x) / (p2y - p1y + 1e-9) + p1x if p2y != p1y else p1x
                if p1x == p2x or x <= xinters:
                    inside = not inside
        p1x, p1y = p2x, p2y
    return inside

if __name__ == '__main__':
    print("User interface:  http://localhost:8000/")
    print("Admin login:     http://localhost:8000/admin")
    app.run(debug=True, port=8000)