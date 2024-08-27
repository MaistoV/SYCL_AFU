import matplotlib.pyplot as plt  # To visualize

operational_intensity = 2
RS_SCHEMA_color = ["r", "g", "b"]
RS_SCHEMA_txt = ["3:2", "6:3", "10:4"]

PCIe_gen3_GB_s=7.877 
# https://www.crucial.com/support/articles-faq-ssd/pcie-speeds-limitations
# https://en.wikipedia.org/wiki/PCI_Express
# https://www.trentonsystems.com/blog/pcie-gen4-vs-gen3-slots-speeds

plt.figure("Roofline",figsize=[16,9])
plt.xlabel("Ops/byte")
plt.ylabel("GOps/second")

# Empirically observed values
throughput_GOPS_samples = [	2.062662702834	,
							1.0188349527224358,
							0.6161680235977565
							]
plt.scatter([2,2,2], throughput_GOPS_samples, c=RS_SCHEMA_color, marker="x")

# Modeled values
NUM_OPS_PER_BYTE=2
throughput_PCIe_gen3_GB_s_peak = [PCIe_gen3_GB_s/(3+1), PCIe_gen3_GB_s/(6+1), PCIe_gen3_GB_s/(10+1)]
throughput_GOPS_peak = [0 for _ in range(0,3)]
for rs in range(0, 3):
	throughput_GOPS_peak[rs] =  NUM_OPS_PER_BYTE*throughput_PCIe_gen3_GB_s_peak[rs]

# Peak memory bandwidth
throughput_GOPS_divided_by_PCIe_gen3_GB_s = [0 for _ in range(0,3)]
for rs in range(0,len(RS_SCHEMA_txt)):
	throughput_GOPS_divided_by_PCIe_gen3_GB_s[rs] = throughput_GOPS_peak[rs]/PCIe_gen3_GB_s
max_y = (int(throughput_GOPS_divided_by_PCIe_gen3_GB_s[0])+1) /10
x = [0. for _ in range(0,max_y)]
y = [0. for _ in range(0,max_y)]
for i in range(0,max_y):
	x[i] = i/10
	y[i] = PCIe_gen3_GB_s*x[i]
plt.plot(x,y) 

# Peak GOPS
for rs in range(0,len(RS_SCHEMA_txt)):
	plt.hlines(throughput_GOPS_peak[rs], xmin=throughput_GOPS_divided_by_PCIe_gen3_GB_s[rs], xmax=5, 
	    colors=RS_SCHEMA_color[rs], label=RS_SCHEMA_txt[rs])
plt.xticks([operational_intensity],str(operational_intensity))
plt.xlim([0,5])
plt.grid(visible=True, which="both")

# RS Operational intensity
plt.vlines([operational_intensity], ymin=1, ymax=PCIe_gen3_GB_s*operational_intensity, linestyles="dashed")

plt.legend()
plt.savefig("Roofline")

plt.show()