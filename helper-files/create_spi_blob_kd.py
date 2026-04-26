import json
import struct

NAME_MAX_BYTES = 32

def encode_name(name, max_bytes=NAME_MAX_BYTES):

    encoded = name.encode("utf-8")

    if len(encoded) > max_bytes - 1:
        truncated = name.encode("utf-8")[:max_bytes - 4]
        encoded = truncated + b"..."

    return encoded.ljust(max_bytes, b"\x00")

def build_kd_tree(points, depth=0):

    if not points:
        return None

    axis = depth % 2
    points.sort(key=lambda p: p[axis])
    median = len(points) // 2

    return {
        "point": points[median],
        "left":  build_kd_tree(points[:median], depth + 1),
        "right": build_kd_tree(points[median + 1:], depth + 1),
    }

def write_kd_tree(node, f):
    if node is None:
        return

    lat, lon, name = node["point"]

    has_left = 1 if node["left"]  is not None else 0
    has_right = 1 if node["right"] is not None else 0

    f.write(struct.pack("dd", lat, lon))
    f.write(encode_name(name))
    f.write(struct.pack("BB", has_left, has_right))

    write_kd_tree(node["left"],  f)
    write_kd_tree(node["right"], f)

def parse_geojson(input_file):

    with open(input_file, "r", encoding="utf-8") as f:
        data = json.load(f)

    points = []

    for feature in data.get("features", []):

        geometry = feature.get("geometry", {})
        name = feature.get("properties", {}).get("name", "Unknown")

        if geometry.get("type") == "Point":

            lon, lat = geometry["coordinates"]
            points.append((lat, lon, name))

    return points

def main():

    input_file  = "./florida_liquor_stores.json"
    output_file = "liquor_stores.kd"

    points = parse_geojson(input_file)

    tree = build_kd_tree(points)

    with open(output_file, "wb") as f:

        f.write(struct.pack("I", len(points)))  
        write_kd_tree(tree, f)

if __name__ == "__main__":
    main()

