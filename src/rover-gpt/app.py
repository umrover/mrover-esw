from flask import Flask
from flask import request
import time
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')
from matplotlib.backends.backend_pdf import PdfPages
import ollama


all_data = {
    "uv": [(0, 10), (1, 12), (2, 14), (3, 13), (4, 15), (5, 16)],
    "methane": [(0, 0), (1, 2), (2, 3), (3, 5), (4, 8), (5, 13)],  # increasing trend
    "oxygen": [(0, 21), (1, 20.5), (2, 20), (3, 19.8), (4, 19.5), (5, 19.3)],  # decreasing trend
    "temp": [(0, 22), (1, 24), (2, 26), (3, 25), (4, 27), (5, 29)],  # rising with small dips
    "humidity": [(0, 45), (1, 47), (2, 50), (3, 48), (4, 52), (5, 55)]  # fluctuating trend
}

t0 = time.time()

app = Flask(__name__)

@app.route('/')
def hello_world():
    return "RoverGPT"

@app.route('/uv_data')
def handle_incoming_uv():
    ct = time.time()
    val = request.args.get('value', type = float)
    all_data["uv"].append((ct - t0, val))
    return f"Received at {ct - t0} seconds : {val} index for uv"

@app.route('/methane_data')
def handle_incoming_methane():
    ct = time.time()
    val = request.args.get('value', type = float)
    all_data["methane"].append((ct - t0, val))
    return f"Received at {ct - t0} seconds : {val} ppm for methane"

@app.route('/oxygen_data')
def handle_incoming_oxygen():
    ct = time.time()
    val = request.args.get('value', type = float)
    all_data["uv"].append((ct - t0, val))
    return f"Received at {ct - t0} seconds : {val} index for uv"

@app.route('/temp_data')
def handle_incoming_temp():
    ct = time.time()
    val = request.args.get('value', type = float)
    all_data["temp"].append((ct - t0, val))
    return f"Received at {ct - t0} seconds : {val} degrees for temperature"

@app.route('/humidity_data')
def handle_incoming_humidity():
    ct = time.time()
    val = request.args.get('value', type = float)
    all_data["humidity"].append((ct - t0, val))
    return f"Received at {ct - t0} seconds : {val} % humidity"


@app.route('/graph')
def generate_graphs():
    # Create PdfPages object with direct file output
    with PdfPages('output.pdf') as pdf:
        # Calculate number of rows and columns for subplot grid
        n_plots = len(all_data)
        n_rows = (n_plots + 1) // 2  # 2 plots per row
        n_cols = min(2, n_plots)
        
        # Create figure with subplots
        fig = plt.figure(figsize=(15, 5 * n_rows))
        
        # Create each subplot
        for idx, (category, data) in enumerate(all_data.items(), 1):
            # Extract times and values
            times, values = zip(*data)
            
            # Create subplot
            ax = fig.add_subplot(n_rows, n_cols, idx)
            
            # Plot data
            ax.plot(times, values, marker='o')
            
            # Customize plot
            ax.set_title(f'{category.capitalize()} Over Time')
            ax.set_xlabel('Time')
            ax.set_ylabel(category.capitalize())
            ax.grid(True)
            
            # Rotate x-axis labels for better readability
            plt.setp(ax.get_xticklabels(), rotation=45)
        
        # Adjust layout to prevent overlap
        plt.tight_layout()
        
        # Save the current figure to PDF
        pdf.savefig(fig)
        plt.close()
      
    response = ollama.chat(model='phi3', messages=[
      {
        'role': 'user',
        'content': f"{all_data} Read the following sensor data over time and write a summmary of the general trends you see for each data. Bullet point your answer with a max of 3 bullet points and each bullet point should be no more than 20 words.",
      },
    ])
    return response['message']['content']

print("Graph generated & saved to pdf")