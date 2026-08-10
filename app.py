from flask import Flask, render_template, request, redirect, url_for
import subprocess

app = Flask(__name__)

HEAP_FILE = "/Users/sakshibisht/Documents/project/heap_state.dat"
GRAPH_FILE = "/Users/sakshibisht/Documents/project/graph_state.dat"

@app.route("/", methods=["GET", "POST"])
def index():
    outputs = {}

    if request.method == "POST":
        # Allocate memory
        if "allocate_submit" in request.form:
            size = request.form["size"]
            result = subprocess.run(["./backend_main", "allocate", size], capture_output=True, text=True)
            outputs["allocate"] = result.stdout

        # Deallocate memory
        elif "deallocate_submit" in request.form:
            pid = request.form["pid"]
            result = subprocess.run(["./backend_main", "deallocate", pid], capture_output=True, text=True)
            outputs["deallocate"] = result.stdout

        # Compact memory
        elif "compact_submit" in request.form:
            result = subprocess.run(["./backend_main", "compact"], capture_output=True, text=True)
            outputs["compact"] = result.stdout

        # Add process relation
        elif "relation_submit" in request.form:
            from_pid = request.form["from_pid"]
            to_pid = request.form["to_pid"]
            result = subprocess.run(["./backend_main", "add_relation", from_pid, to_pid], capture_output=True, text=True)
            outputs["relation"] = result.stdout

        # Show process graph
        elif "graph_submit" in request.form:
            result = subprocess.run(["./backend_main", "show_graph"], capture_output=True, text=True)
            outputs["graph"] = result.stdout

        # View memory
        elif "view_submit" in request.form:
            result = subprocess.run(["./backend_main", "display"], capture_output=True, text=True)
            outputs["display"] = parse_memory_table(result.stdout)

    return render_template("index.html", outputs=outputs)


@app.route("/logout_action", methods=["POST"])
def logout_action():
    subprocess.run(["./backend_main", "logout"], capture_output=True, text=True)
    return redirect(url_for("index"))


def parse_memory_table(output):
    """
    Parses tabular heap display output from backend_main into
    a list of dicts for rendering in HTML table.
    """
    rows = []
    lines = output.strip().split("\n")

    for line in lines:
        parts = line.split()
        # Skip header and separator lines
        if len(parts) == 5 and parts[0].isdigit():
            rows.append({
                "pid": parts[0],
                "start": parts[1],
                "end": parts[2],
                "size": parts[3],
                "status": parts[4],
            })
    return rows


if __name__ == "__main__":
    app.run(debug=True)
