import sys
import numpy as np
import matplotlib.pyplot as plt  # To visualize
import pandas  # To read data
from sklearn.linear_model import LinearRegression

# Utility Constants
KB = 1024
MB = 1024 * KB
GB = 1024 * MB

cell_length = [
                "64B"	, "128B"  ,	"256B"  ,	"512B"  ,
                "1KB"   , "2KB"   , "4KB"   ,   "8KB"   ,   "16KB"
            ]
cell_length_int = [
				64	  ,	128   ,	256   ,   512	,
                1*KB  , 2*KB  , 4*KB  ,   8*KB  ,   16*KB
            ]
RS_SCHEMA_list = ["RS_3_2", "RS_6_3"]
RS_SCHEMA_color = ["g", "b"]
RS_SCHEMA_txt = ["3:2", "6:3"]
FMAX_MHz = 312.5
# FMAX_MHz = 600.


# Source data directory
root_data_dir = "../data"
if len(sys.argv) >= 2:
	root_data_dir = sys.argv[1]

# Output directory for plots
plot_dir = "./output_plots"
if len(sys.argv) >= 3:
	plot_dir = sys.argv[2]

plt.figure("Clock cyles latency vs Cell length", figsize=[15,10])
plt.title("Clock cyles latency vs Cell length")
# ax = plt.subplot(1,2,1)
model = [0. for _ in range(0, len(RS_SCHEMA_list)) ]
label_linregr = ["" for _ in range(0, len(RS_SCHEMA_list)) ]
throughput_B_s_peak = [0 for _ in range(0, len(RS_SCHEMA_list))]
throughput_B_cycle = [[0 for _ in range(len(cell_length)) ] for _ in range(len(RS_SCHEMA_list))]
throughput_B_s = [[0 for _ in range(len(cell_length)) ] for _ in range(len(RS_SCHEMA_list))]
for rs in range(0, len(RS_SCHEMA_list)):
    # ax = plt.subplot(1,2,rs+1, sharey=ax)

    datafile = root_data_dir + "/cycles_" + RS_SCHEMA_list[rs] + ".csv"
    print("Reading file " + datafile)
    # Load data
    data = pandas.read_csv(datafile, sep=",")
    data = data.sort_values(by=["Cell length"])
    # Get lengths for linar regression
    x = data["Cell length"].values .reshape(-1, 1)
	# Get unique values for plots
    # x_unique = pandas.unique(x[:,0]).reshape(-1, 1)
    # Reshaping with -1 means to calculate dimension of rows, but have 1 column
    cycles_min = data["min"].values.reshape(-1, 1)
    cycles_max = data["max"].values.reshape(-1, 1)
    cycles_avg = data["avg"].values.reshape(-1, 1)
    model[rs] = LinearRegression().fit(x, cycles_avg)
    cycles_pred = model[rs].predict(np.array(cell_length_int).reshape(-1, 1))
    print("cycles_pred: ")
    print(cycles_pred)
    label_linregr[rs] = f'{model[rs].intercept_[0]:.2f} + {model[rs].coef_[0][0]:.2f} * cell_length'

    # Theoretical peak throughput at FMAX_MHz
	#   Throughput = cell_length / latency
	#   Throughput = cell_length / (cycles * frequency)
	#   Throughput = frequency * cell_length / cycles
	#   Throughput = frequency * cell_length / (intercept_ + (coef_ * cell_length))
	# If coef_ is statistically significant, and cell_length >> intercept_, then:
	#   Throughput = frequency * cell_length / (coef_ * cell_length))
	#   Throughput = frequency * cell_length / cell_length / coef_
    #     Throughput = freq / coef_
    throughput_B_s_peak[rs] = (1. / model[rs].coef_[0][0])*FMAX_MHz*1000000
    # Save throughput byte/cycle
    for l in range(0,len(cell_length)):
        throughput_B_cycle[rs][l] = cell_length_int[l] / cycles_pred[l]
        throughput_B_s[rs][l] = throughput_B_cycle[rs][l]*FMAX_MHz*1000000

    # plt.subplot(1,3,rs+1, sharey=ax, sharex=ax)
    # plt.title( "RS[" + RS_SCHEMA_txt[rs] + "]" )
    # plt.scatter(x, cycles_min)#, label="Min")
    # plt.scatter(x, cycles_max)#, label="Max")
    # plt.scatter(x, cycles_avg)#, label="Avg")
    # plt.plot(x, cycles_avg, "-o", label="Avg " + RS_SCHEMA_list[rs])

    plt.plot(cell_length_int, cycles_pred, label="RS[" + RS_SCHEMA_txt[rs] + "] " + label_linregr[rs], color=RS_SCHEMA_color[rs])
    # Set log-scales
    # plt.xscale("log", base=2)
    # plt.yscale("log", base=10)
    plt.legend()
    plt.xlabel("Cell length")
    plt.grid(True, axis="both")
    plt.ylabel("Clock cycles latency")
    plt.xticks(cell_length_int, cell_length, rotation=45)

figname = plot_dir + "/Clock_cyles_latency.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")

print("Cycles_model:")
for rs in range(0, len(RS_SCHEMA_list)):
    print("    " + RS_SCHEMA_txt[rs], ": Cycles = " + label_linregr[rs])
print("Throughput_model:")
for rs in range(0, len(throughput_B_s_peak)):
    print("    " + RS_SCHEMA_txt[rs], ": at " + str(FMAX_MHz) + "MHz = ", throughput_B_s_peak[rs]/KB/KB/KB, " GB/s")
    print("    Peak: " + str(throughput_B_s_peak[rs]/GB) + " GB/s")

B_s		= [ "1", "2", "4", "6", "8", "10", "12"]
B_s_int = [ GB, 2*GB, 4*GB, 6*GB, 8*GB, 10*GB, 12*GB]
plt.figure("Peak_throughput_seconds", figsize=[15,15])
plt.bar( RS_SCHEMA_txt, np.array(throughput_B_s_peak).reshape(len(throughput_B_s_peak)), log=True)
plt.ylabel("GB/s")
# plt.yticks(B_s_int, B_s)
plt.grid(visible=True, which="both")
figname = plot_dir + "/Peak_throughput_seconds.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")

plt.figure("Troughput_cycle", figsize=[15,15])
for rs in range(0,len(RS_SCHEMA_list)):
	plt.loglog(cell_length_int, throughput_B_cycle[rs],  RS_SCHEMA_color[rs]+"-o", label="RS[" + RS_SCHEMA_txt[rs] + "]", base=2, linewidth=2)
plt.grid(visible=True, which="both")
plt.xticks(cell_length_int, cell_length)
plt.xlabel("Cell length")
plt.ylabel("Troughput (B/cycle)")
plt.legend()
figname = plot_dir + "/Troughput_cycle.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")

plt.figure("Troughput_seconds", figsize=[15,15])
for rs in range(0,len(RS_SCHEMA_list)):
    plt.loglog(cell_length_int, throughput_B_s[rs],  RS_SCHEMA_color[rs]+"-o", label="RS[" + RS_SCHEMA_txt[rs] + "]",   base=2, linewidth=2)
    plt.axhline(y = throughput_B_s_peak[rs], linestyle='--', color=RS_SCHEMA_color[rs]) # Vertical line at 1MB
# plt.hlines(throughput_B_s_peak, xmin=cell_length_int[0], xmax=cell_length_int[-1], colors=RS_SCHEMA_color, linestyles="dashed")
plt.grid(visible=True, which="both")
plt.xticks(cell_length_int, cell_length)
plt.xlabel("Cell length")
plt.ylabel("Throughput (GB/s)")
plt.yticks(B_s_int, B_s)
plt.legend()
figname = plot_dir + "/Troughput_seconds.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")


plt.show()
