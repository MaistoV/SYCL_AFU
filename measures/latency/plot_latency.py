import matplotlib.pyplot as plt
import glob
import pandas
import numpy
# from sklearn.linear_model import LinearRegression
# import sys
import os

data_dirs = ["", ""]

# hw_configs = ["ISA-L", "SYCL_ASP", SYCL_AFU]
hw_configs = ["ISA-L", "SYCL_ASP"]
# hw_configs = ["ISA-L"]
ISA_L		= 0
SYCL_ASP  	= 1
SYCL_AFU  	= 2
data_dirs[ISA_L]	= "./data/data_ISA_L/"
data_dirs[SYCL_ASP] = "./data/data_SYCL_ASP/"
# data_dirs[SYCL_AFU] = "./data/data_SYCL_AFU/"

# Output directory for plots
plot_dir = "./plots"
os.makedirs(plot_dir, exist_ok=True)

# RS_SCHEMA_list = ["3_2", "6_3", "10_4"]
# RS_SCHEMA_txt  = ["3:2", "6:3", "10:4"]
RS_SCHEMA_list = ["3_2", "6_3"]
RS_SCHEMA_txt  = ["3:2", "6:3" ]
RS_SCHEMA_color = ["r", "g", "b"]
# RS_10_4 = 2
RS_6_3 = 1
RS_3_2 = 0

cell_length = [ 
				# "64B"	,	"128B",	"256B",	"512B", 
				"1KB"	,	"2KB"	,	"4KB"	,	"8KB"	,	"16KB"	,
				"32KB"	,	"64KB"	,	"128KB"	, 	"256KB"	,	"512KB"	,
				"1MB"	,	"2MB"	,	"4MB"	,	"8MB"	, 	"16MB"	,
				 "32MB"	,	"64MB"#	,	"128MB"	, 	"256MB"	,	"512MB"	,
				# "1GB"
				]
cell_length_int = [# 64, 128, 256, 512, 
					1024           , 2*1024         , 4*1024         , 8*1024       , 16*1024 		,
					32*1024        , 64*1024        , 128*1024       , 256*1024     , 512*1024 		,
					1024*1024      , 2*1024*1024    , 4*1024*1024    , 8*1024*1024  , 16*1024*1024 	,
					32*1024*1024  , 64*1024*1024   #, 128*1024*1024  , 256*1024*1024, 512*1024*1024 ,
					# 1024*1024*1024 	
				]
index_1MB = 10

# Preallocate arrays
mean_latency_s 		= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]
# latency_s 			= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]
throughput_B_s 		= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]
afu_latency_s 		= [0. for _ in range(len(cell_length)) ]
# median_latency_s 		= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]
# throughput_B_s_median 	= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]

# Figure Linear regression
for hw in range(0,len(hw_configs)):
	for rs in range(0,len(RS_SCHEMA_list)):
		for l in range(0,len(cell_length)):
			# Compose filename
			afu_latency_s_file_name = data_dirs[hw] + 'latency_' + RS_SCHEMA_list[rs] + "_" + cell_length[l] + "_" + hw_configs[hw] + '.txt'
			file_name_ref = glob.glob(afu_latency_s_file_name)
			if ( len(file_name_ref) != 1 ): 
				print("File name error: " + afu_latency_s_file_name)
				continue

			# Load data
			afu_latency_s = pandas.read_csv(file_name_ref[0], sep=";", header=None)
			# latency_s[rs][hw][l] = afu_latency_s

			# Save mean_latency_s
			mean_latency_s[rs][hw][l] = numpy.average(afu_latency_s)
			# mean_latency_s[rs][hw][l] = numpy.median(afu_latency_s)

			# Save throughput byte/second
			throughput_B_s[rs][hw][l] = cell_length_int[l] / mean_latency_s[rs][hw][l]


# 		# Perform linear regression
# 		x = numpy.array(cell_length_int).reshape(-1, 1)
# 		model = LinearRegression().fit(x, numpy.array(mean_latency_s[rs][hw]).reshape(-1, 1))
# 		y_pred = model.predict(x)
# 		print(RS_SCHEMA_list[rs], " ", hw_configs[hw], ": Latency = ", model.intercept_ , " + ", model.coef_, " * cell_length")
# 		plt.plot(cell_length_int, y_pred, label="RS[" + RS_SCHEMA_txt[rs] + "] " + hw_configs[hw])
# 		plt.scatter(x, mean_latency_s[rs][hw], marker="x")
# 		plt.xticks(cell_length_int, cell_length)
# plt.ylabel("ms")
# plt.xlabel("Cell length")
# plt.grid(visible=True, which="both")
# plt.legend()
# plt.savefig(plot_dir + "/" + "Linear regression", dpi=400, bbox_inches="tight")
#S
# Raw histograms
# for rs in range(0,len(RS_SCHEMA_list)):
# 	# plt.figure("RS[" + RS_SCHEMA_txt[rs] + "]")
# 	# for hw in range(0,len(hw_configs)):
# 	pandas.DataFrame(latency_s[rs][0][len(cell_length)-1]).hist()
# plt.show()
# exit()

# Figure Latency
plt.figure("Latency", figsize=[16,9])
# for rs in range(0,len(RS_SCHEMA_list)):
# 	plt.loglog(cell_length_int, mean_latency_s[rs][SYCL_ASP  ], RS_SCHEMA_color[rs]+"-o", label="RS[" + RS_SCHEMA_txt[rs] + "] SYCL_ASP",   linewidth=2)
for rs in range(0,len(RS_SCHEMA_list)):
	plt.loglog(cell_length_int, mean_latency_s[rs][ISA_L], RS_SCHEMA_color[rs]+"-x", label="RS[" + RS_SCHEMA_txt[rs] + "] ISA-L"	)
plt.xlabel("Cell length")
plt.ylabel("ms")
plt.xticks(cell_length_int, cell_length)
plt.grid(visible=True)
plt.legend()
plt.savefig(plot_dir + "/" + "Latency", dpi=400, bbox_inches="tight")

# # Figure Median Latency
# plt.figure("Median Latency", figsize=[16,9])
# for rs in range(0,len(RS_SCHEMA_list)):
	# plt.loglog(cell_length_int, median_latency_s[rs][SYCL_ASP  ], RS_SCHEMA_color[rs]+"-o", label="RS[" + RS_SCHEMA_txt[rs] + "] SYCL_ASP",   linewidth=2)
# for rs in range(0,len(RS_SCHEMA_list)):
	# plt.loglog(cell_length_int, median_latency_s[rs][ISA_L], RS_SCHEMA_color[rs]+"-x", label="RS[" + RS_SCHEMA_txt[rs] + "] ISA-L", linestyle='dashed'	)
# plt.xlabel("Cell length")
# plt.ylabel("ms")
# plt.xticks(cell_length_int, cell_length)
# plt.grid(visible=True)
# plt.legend()
# plt.savefig(plot_dir + "/" + "Latency_median", dpi=400, bbox_inches="tight")
#
# Figure SYCL_ASP / ISA-L slowdown
# plt.figure("Relative slowdown SYCL_ASP / ISA-L", figsize=[16,9])
# plt.title("Relative slowdown SYCL_ASP / ISA-L")
# print("RS[K:P],W,Slowdown SYCL_ASP/ISA-L")
# slowdown = [[0. for _ in range(len(cell_length)) ] for _ in range(len(RS_SCHEMA_list))]
# for rs in range(0,len(RS_SCHEMA_list)):
# 	for l in range(0,len(cell_length)):
# 		# Compute relative speedup SYCL_ASP / ISA-L
# 		slowdown[rs][l] = mean_latency_s[rs][SYCL_ASP][l] / mean_latency_s[rs][ISA_L][l]
# 		# printRS_SCHEMA_txt[rs], ",", cell_length[l], ",", slowdown[rs][l]
# 	plt.loglog(cell_length_int, slowdown[rs],  RS_SCHEMA_color[rs]+"-o", label="RS " + RS_SCHEMA_list[rs] )
# 	print(RS_SCHEMA_txt[rs], ",", cell_length[index_1MB], ",", slowdown[rs][index_1MB])
# plt.grid(visible=True, which="both")
# plt.xticks(cell_length_int, cell_length)
# plt.xlabel("Cell length")
# plt.ylabel("SYCL_ASP/ISA-L latency")
# # plt.ylim(bottom=1) 
# plt.legend()
# plt.savefig(plot_dir + "/" + "Relative_slowdown_SYCL_vs_ISA-L", dpi=400, bbox_inches="tight")
#
# Figure ISA-L / SYCL_ASP slowdown
# plt.figure("Relative slowdown ISA-L / SYCL_ASP", figsize=[16,9])
# plt.title("Relative slowdown ISA-L / SYCL_ASP")
# print("RS[K:P],W,Slowdown ISA-L / SYCL_ASP")
# slowdown = [[0. for _ in range(len(cell_length)) ] for _ in range(len(RS_SCHEMA_list))]
# for rs in range(0,len(RS_SCHEMA_list)):
# 	for l in range(0,len(cell_length)):
# 		# Compute relative speedup ISA-L / SYCL_ASP
# 		slowdown[rs][l] = mean_latency_s[rs][ISA_L][l] / mean_latency_s[rs][SYCL_ASP][l]
# 		# printRS_SCHEMA_txt[rs], ",", cell_length[l], ",", slowdown[rs][l]
# 	plt.loglog(cell_length_int, slowdown[rs],  RS_SCHEMA_color[rs]+"-o", label="RS " + RS_SCHEMA_list[rs] )
# 	print(RS_SCHEMA_txt[rs], ",", cell_length[index_1MB], ",", slowdown[rs][index_1MB])
# plt.grid(visible=True, which="both")
# plt.xticks(cell_length_int, cell_length)
# plt.xlabel("Cell length")
# plt.ylabel("ISA-L/SYCL_ASP latency")
# # plt.ylim(top=1) 
# plt.legend()
# plt.savefig(plot_dir + "/" + "Relative_slowdown_ISA-L_vs_SYCL", dpi=400, bbox_inches="tight")

# Figure Throughput
B_s		= [ "100MB/s", "1GB/s", "10GB/s"]
B_s_int = [ 100*1024*1024, 1024*1024*1024, 10*1024*1024*1024]
plt.figure("Throughput", figsize=[16,9])
# for rs in range(0,len(RS_SCHEMA_list)):
# 	plt.loglog(cell_length_int, throughput_B_s[rs][SYCL_ASP  ],  RS_SCHEMA_color[rs]+"-o", label="RS[" + RS_SCHEMA_txt[rs] + "] SYCL_ASP",   base=2, basey=10, linewidth=2)
# 	print(throughput_B_s[rs][SYCL_ASP  ][len(cell_length)-1]/1024/1024/1024)
for rs in range(0,len(RS_SCHEMA_list)):
	plt.loglog(cell_length_int, throughput_B_s[rs][ISA_L],  RS_SCHEMA_color[rs]+"x", label="RS[" + RS_SCHEMA_txt[rs] + "] ISA-L", linestyle='dashed'	)
	ax = plt.gca(); ax.set_xscale("log", base=2); ax.set_yscale("log", base=10)
plt.grid(visible=True, which="both")
plt.yticks(B_s_int, B_s)
plt.xticks(cell_length_int, cell_length)
plt.xlabel("Cell length")
plt.ylabel("Throughput (B/s)")
plt.legend()
plt.savefig(plot_dir + "/" + "Throughput", dpi=400, bbox_inches="tight")

# Figure RS slowdown
# plt.figure("RS slowdown", figsize=[16,9])
# code_latency_ratio_6_3_SYCL 		= [0. for _ in range(len(cell_length))]
# code_latency_ratio_10_4_SYCL 	= [0. for _ in range(len(cell_length))]
# code_latency_ratio_6_3_ISAL 	= [0. for _ in range(len(cell_length))]
# code_latency_ratio_10_4_ISAL 	= [0. for _ in range(len(cell_length))]
# for l in range(0,len(cell_length)):
# 	# Compute relative speedup VS RS[3:2]
# 	code_latency_ratio_6_3_SYCL 		[l] 	= mean_latency_s[RS_6_3 ][SYCL_ASP  ][l] / mean_latency_s[RS_3_2][SYCL_ASP  ][l]
# 	code_latency_ratio_10_4_SYCL 	[l] 	= mean_latency_s[RS_10_4][SYCL_ASP  ][l] / mean_latency_s[RS_3_2][SYCL_ASP  ][l]
# 	code_latency_ratio_6_3_ISAL 	[l] 	= mean_latency_s[RS_6_3 ][ISA_L][l] / mean_latency_s[RS_3_2][ISA_L][l]
# 	code_latency_ratio_10_4_ISAL 	[l] 	= mean_latency_s[RS_10_4][ISA_L][l] / mean_latency_s[RS_3_2][ISA_L][l]

# plt.semilogx(cell_length_int, code_latency_ratio_6_3_SYCL 	, RS_SCHEMA_color[RS_6_3 ]+"-o", label="RS[6:3]")
# plt.semilogx(cell_length_int, code_latency_ratio_10_4_SYCL 	, RS_SCHEMA_color[RS_10_4]+"-o", label="RS[10:4]")
# plt.semilogx(cell_length_int, code_latency_ratio_6_3_ISAL 	, RS_SCHEMA_color[RS_6_3 ]+"-x", label="RS[6:3]"	, linestyle='dashed')
# plt.semilogx(cell_length_int, code_latency_ratio_10_4_ISAL 	, RS_SCHEMA_color[RS_10_4]+"-x", label="RS[10:4]"	, linestyle='dashed')
# plt.xticks(cell_length_int, cell_length)
# plt.hlines(6/3., xmin=cell_length_int[0], xmax=cell_length_int[-1], colors=RS_SCHEMA_color[RS_6_3], label="K ratio 6/3")
# plt.hlines(10/3., xmin=cell_length_int[0], xmax=cell_length_int[-1],colors=RS_SCHEMA_color[RS_10_4], label="K ratio 10/3")
# plt.xlabel("Cell length")
# plt.ylabel("Slowdown w.r.t. RS[3:2]")
# plt.legend()
# plt.savefig(plot_dir + "/" + "RS slowdown", dpi=400, bbox_inches="tight")

# plt.show()

print("Plots are available at " + plot_dir)