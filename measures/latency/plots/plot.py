import matplotlib.pyplot as plt
import glob
import pandas
import numpy
import sys
import os
import plot_common as common

# Source data directory
root_data_dir = "../data/"
if len(sys.argv) >= 1:
	root_data_dir = sys.argv[1]

# Output directory for plots
plot_dir = "./output_plots"
if len(sys.argv) >= 2:
	plot_dir = sys.argv[2]

# Create output directory
os.makedirs(plot_dir, exist_ok=True)

#############
# Read data #
#############
# Preallocate arrays
mean_latency_s 		= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(common.hw_configs))] for _ in range(len(common.RS_SCHEMA_list))]
# latency_s 			= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(common.hw_configs))] for _ in range(len(common.RS_SCHEMA_list))]
throughput_B_s 		= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(common.hw_configs))] for _ in range(len(common.RS_SCHEMA_list))]
afu_latency_s 		= [0. for _ in range(len(common.cell_length)) ]

for hw in range(0,len(common.hw_configs)):
	for rs in range(0,len(common.RS_SCHEMA_list)):
		for l in range(0,len(common.cell_length)):
			# Compose filename, must match filenames from this project and VFProxy, e.g.:
			# - latency_3_2_1KB_SYCL_AFU
			# - latency_RS_3_2_numVFP1_numClients1_clientID0_1KB
			afu_latency_s_file_name = root_data_dir + common.data_dirs[hw] + 'latency_*' + common.RS_SCHEMA_list[rs] + "*_" + common.cell_length[l] + '*.txt'
			file_name_ref = glob.glob(afu_latency_s_file_name)
			if ( len(file_name_ref) != 1 ): 
				print("File name error: " + afu_latency_s_file_name)
				afu_latency_s = numpy.inf
				continue
			
			# Load data
			try:
				afu_latency_s = pandas.read_csv(file_name_ref[0], sep=";", header=None)
				# latency_s[rs][hw][l] = afu_latency_s
			except:
				print("File name error: " + afu_latency_s_file_name)
				afu_latency_s = numpy.inf

			# Save mean_latency_s
			# mean_latency_s[rs][hw][l] = numpy.average(afu_latency_s)
			mean_latency_s[rs][hw][l] = numpy.median(afu_latency_s)

			# Save throughput byte/second
			throughput_B_s[rs][hw][l] = common.cell_length_int[l] / mean_latency_s[rs][hw][l]
			# Multiply for P for multi-erasure reconstruction
			if os.environ['MULTI_ERASURE_SIMPLE'] == "1":
				throughput_B_s[rs][hw][l] *= common.RS_P_list[rs]

##################
# Figure Latency #
##################
plt.figure("Latency", figsize=common.figsize_2columns)
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(common.RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	for hw in range(0,len(common.hw_configs)):
		plt.loglog(
					common.cell_length_int, 
			 		mean_latency_s[rs][hw],
					common.hw_line[hw] + common.hw_marker[hw],
					label=common.hw_name[hw],
					color=common.hw_color[hw],
					linewidth=common.hw_linewidth[hw]
				)
	# Decorating
	plt.title(common.RS_SCHEMA_txt[rs])
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.xlabel("Cell length")
	plt.ylabel("Seconds")
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45, minor=False)
	plt.grid(visible=True) #, which="both")
	plt.legend()
figname = plot_dir + "/" + "Latency" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)


plt.figure("Throughput", figsize=common.figsize_2columns)
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(common.RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	plt.axhline(y=common.peak_phy_throughput[rs], linestyle='--' , color="r", linewidth=2, label="Max PCIe read bandwidth")
	# plt.axhline(y=common.peak_asp_throughput[rs], linestyle='--', color="r", linewidth=2, label="Max ASP read bandwidth")
	for hw in range(0,len(common.hw_configs)):
		plt.loglog(
					common.cell_length_int, 
			 		throughput_B_s[rs][hw],
					common.hw_line[hw] + common.hw_marker[hw],
					label=common.hw_name[hw],
					color=common.hw_color[hw],
					linewidth=common.hw_linewidth[hw]
				)
		# print(throughput_B_s[rs][hw][len(common.cell_length)-1]/GB)

	# Decoration
	ax = plt.gca(); ax.set_xscale("log", base=2); ax.set_yscale("log", base=10)
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.grid(visible=True, which="both")
	plt.title(common.RS_SCHEMA_txt[rs])
	plt.yticks(common.B_s_int, common.B_s)
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45)
	plt.xlabel("Cell length")
	plt.ylabel("Throughput (B/s)")
	plt.legend()
figname = plot_dir + "/" + "Throughput" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

exit()

# Print difference between VFProxy and SYCL_AFU
VFP_diff_overhead 		= [[0. for _ in range(len(common.cell_length)) ] for _ in range(len(common.cell_length))]
VFP_percentage_overhead = [[0. for _ in range(len(common.cell_length)) ] for _ in range(len(common.cell_length))]
for rs in range(0,len(common.RS_SCHEMA_list)):
	for l in range(0,len(common.cell_length)):
		VFP_diff_overhead[rs][l]       = mean_latency_s[rs][common.VFProxy][l] - mean_latency_s[rs][common.SYCL_AFU][l]
		VFP_percentage_overhead[rs][l] = mean_latency_s[rs][common.VFProxy][l] / mean_latency_s[rs][common.SYCL_AFU][l]
		
plt.figure("VFProxy Overhead", figsize=common.figsize_2columns)
for rs in range(0,len(common.RS_SCHEMA_list)):
	# print ("VFP_diff_overhead       RS[" + common.RS_SCHEMA_txt[rs] + "]")
	# print (VFP_diff_overhead[rs])
	
	plt.subplot(1, 2, 1)
	plt.title("Abosulte Difference Overhead (VFProxy - SYCL_AFU)")
	plt.semilogx(
		common.cell_length_int,
		VFP_diff_overhead[rs], color=common.RS_color[rs],
		label=common.RS_SCHEMA_txt[rs],
		linewidth=2
	)
	# Decorate
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.grid(visible=True, which="both")
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45)
	plt.legend()
	plt.ylabel("Seconds")

	plt.subplot(1, 2, 2)
	plt.title("Percentage Overhead (VFProxy / SYCL_AFU)")
	plt.semilogx(
		common.cell_length_int,
		VFP_percentage_overhead[rs], color=common.RS_color[rs],
		label=common.RS_SCHEMA_txt[rs],
		linewidth=2
	)
	# Decorate
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.grid(visible=True, which="both")
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45)

figname = plot_dir + "/" + "VFProxy_Overhead" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

print("Plots are available at " + plot_dir)
