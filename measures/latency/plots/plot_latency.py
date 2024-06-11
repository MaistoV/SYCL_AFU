import matplotlib.pyplot as plt
import glob
import pandas
import numpy
import sys
import os
import plot_latency_common as common

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

###########################
# Source data directories #
###########################
data_dirs = ["" for _ in range(len(common.hw_configs)) ]
data_dirs[common.ISA_L   ]	= root_data_dir + "/data_ISA_L/"
data_dirs[common.SYCL_ASP] = root_data_dir + "/data_SYCL_ASP_ASP_ZERO_COPY/"
data_dirs[common.SYCL_AFU] = root_data_dir + "/data_SYCL_AFU/"
# data_dirs[PLAIN_C ] = root_data_dir + "/data_PLAIN_C/"

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
			# Compose filename
			afu_latency_s_file_name = data_dirs[hw] + 'latency_' + common.RS_SCHEMA_list[rs] + "_" + common.cell_length[l] + "_" + common.hw_configs[hw] + '.txt'
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
plt.figure("Latency", figsize=[16,9])
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
					linewidth=common.hw_linewidth[hw]
				)
	# Decorating
	plt.title("RS[" + common.RS_SCHEMA_txt[rs] + "]")
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.xlabel("Cell length")
	plt.ylabel("Seconds")
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45, minor=False)
	plt.grid(visible=True)
	plt.legend()
figname = plot_dir + "/" + "Latency" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)


plt.figure("Throughput", figsize=[16,9])
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(common.RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	plt.axhline(y=common.peak_phy_throughput[rs], linestyle='-' , color="r", linewidth=2, label="Max PCIe read bandwidth")
	plt.axhline(y=common.peak_asp_throughput[rs], linestyle='--', color="r", linewidth=2, label="Max ASP read bandwidth")
	for hw in range(0,len(common.hw_configs)):
		plt.loglog(
					common.cell_length_int, 
			 		throughput_B_s[rs][hw],
					common.hw_line[hw] + common.hw_marker[hw],
					label=common.hw_name[hw],
					linewidth=common.hw_linewidth[hw]
				)
		# print(throughput_B_s[rs][hw][len(common.cell_length)-1]/GB)

	# Decoration
	ax = plt.gca(); ax.set_xscale("log", base=2); ax.set_yscale("log", base=10)
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.grid(visible=True, which="both")
	plt.title("RS[" + common.RS_SCHEMA_txt[rs] + "]")
	plt.yticks(common.B_s_int, common.B_s)
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45)
	plt.xlabel("Cell length")
	plt.ylabel("Throughput (B/s)")
	plt.legend()
figname = plot_dir + "/" + "Throughput" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

print("Plots are available at " + plot_dir)