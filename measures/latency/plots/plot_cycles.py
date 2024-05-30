import sys
import numpy as np
import matplotlib.pyplot as plt  # To visualize
import pandas  # To read data
from sklearn.linear_model import LinearRegression

cell_length = [ "64B"	,	"128B"  ,	"256B"  ,	"512B"  , 
                # "1KB", "2KB"   ,   "4KB"   ,   "8KB"   ,   "16KB"   ,  
				]
cell_length_int = [
				64	    ,	128     ,	256     ,   512	    , 
                # 1*1024  ,   2*1024  ,   4*1024  ,   8*1024  ,   16*1024 
				]
# RS_SCHEMA_list = ["RS_3_2", "RS_6_3"]
RS_SCHEMA_list = ["RS_3_2"]
RS_SCHEMA_color = ["g", "b"]
RS_SCHEMA_txt = ["3:2", "6:3"]
# FMAX_MHz = 312.5
FMAX_MHz = 600.


# Source data directory
root_data_dir = "../data"
if len(sys.argv) >= 1:
	root_data_dir = sys.argv[1]

# Output directory for plots
plot_dir = "./output_plots"
if len(sys.argv) >= 2:
	plot_dir = sys.argv[2]

# plt.figure("Clock cyles latency vs Cell length", figsize=[15,15])
# ax = plt.subplot(1,3,1)
throughput_B_s_peak = [0 for _ in range(0, len(RS_SCHEMA_list))]
throughput_B_cycle = [[0 for _ in range(len(cell_length)) ] for _ in range(len(RS_SCHEMA_list))]
throughput_B_s = [[0 for _ in range(len(cell_length)) ] for _ in range(len(RS_SCHEMA_list))]
print("cycles_model:")
for rs in range(0, len(RS_SCHEMA_list)):
    datafile = root_data_dir + "/cycles_" + RS_SCHEMA_list[rs] + ".csv"

    data = pandas.read_csv(datafile, sep=",")  # load data set
    x = data["Cell length"].values .reshape(-1, 1)
    # Reshaping with -1 means to calculate dimension of rows, but have 1 column
    cycles_min = data["min"].values.reshape(-1, 1)
    cycles_max = data["max"].values.reshape(-1, 1)
    cycles_avg = data["avg"].values.reshape(-1, 1)
    model = LinearRegression().fit(x, cycles_avg)
    cycles_pred = model.predict(x)
    print(RS_SCHEMA_txt[rs], ": Cycles = " + str(model.intercept_[0]) + " + " + str(model.coef_[0][0]) + " * cell_length")

    # Theoretical peak throughput at FMAX_MHz
	#   throughput = cell_length / latency
	#   throughput = cell_length / (cycles * frequency)
	#   throughput = frequency * cell_length / cycles
	#   throughput = frequency * cell_length / (intercept_ + (coef_ * cell_length))
	# If coef_ is statistically significant, and cell_length >> intercept_, then:
	#   throughput = frequency * cell_length / (coef_ * cell_length))
	#   throughput = frequency * cell_length / cell_length / coef_ 
    #     throughput = freq / coef_
    throughput_B_s_peak[rs] = (1. / model.coef_)*FMAX_MHz*1000000
    throughput_B_s_peak[rs] = throughput_B_s_peak[rs][0][0]
    # Save throughput byte/cycle
    for l in range(0,len(cell_length)):
        throughput_B_cycle[rs][l] = cell_length_int[l] / cycles_avg[l]
        throughput_B_s[rs][l] = throughput_B_cycle[rs][l]*FMAX_MHz*1000000

    # plt.subplot(1,3,rs+1, sharey=ax, sharex=ax)
    plt.scatter(x, cycles_min, label="Min")
    plt.scatter(x, cycles_max, label="Max")
    plt.scatter(x, cycles_avg, label="Avg")
    # plt.plot(x, cycles_avg, "-o", label="Avg " + RS_SCHEMA_list[rs])
    plt.plot(x, cycles_pred, label="Linear regression (on Avg)")
    plt.legend()
    plt.title( "RS[" + RS_SCHEMA_txt[rs] + "]" )
    plt.xlabel("Cell length")
    plt.ylabel("Clock cycles latency")
    plt.xticks(x[:,0], cell_length)

figname = plot_dir + "/Clock_cyles_latency.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")


print("Throughput_model:")
for rs in range(0, len(throughput_B_s_peak)):
    print(throughput_B_s_peak[rs])
    print(RS_SCHEMA_txt[rs], ": at " + str(FMAX_MHz) + "MHz = ",throughput_B_s_peak[rs]/1024/1024/1024, " GB/s")

B_s		= [ "1", "2", "4", "6", "8", "10", "12"]
G=1024*1024*1024
B_s_int = [ G, 2*G, 4*G, 6*G, 8*G, 10*G, 12*G]
plt.figure("Peak_throughput_seconds", figsize=[15,15])
plt.bar( RS_SCHEMA_txt, np.array(throughput_B_s_peak).reshape(len(throughput_B_s_peak)), log=True)
plt.ylabel("GB/s")
plt.yticks(B_s_int, B_s)
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
plt.hlines(throughput_B_s_peak, xmin=cell_length_int[0], xmax=cell_length_int[-1], colors=RS_SCHEMA_color, linestyles="dashed")
plt.grid(visible=True, which="both")
plt.xticks(cell_length_int, cell_length)
plt.xlabel("Cell length")
plt.ylabel("Troughput (GB/cycle)")
plt.yticks(B_s_int, B_s)
plt.legend()
figname = plot_dir + "/Troughput_seconds.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")


plt.show()
