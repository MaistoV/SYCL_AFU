class PerformanceModel:
    def __init__(self,
                 u_coder=None, N_coder=None, t_coder=None,
                 u_VFPClient=None, N_VFPClient=None, t_VFPClient=None,
                 t_MP=None, cell_len=None, alpha_VFPClient=None, alpha_VF=None, alpha_len=None, alpha_MP=None,
                 u_VFP=None, t_VFP=None,
                 N_VF=None,
                 N_PAC=None, u_PCIe=None, K=None,
                 u_AFU=None, t_AFU=None, P=None,
                 u_RS=None):
        # HDFS
        self.u_coder = u_coder
        self.N_coder = N_coder
        self.t_coder = t_coder

        # VFPClient
        self.u_VFPClient = u_VFPClient
        self.N_VFPClient = N_VFPClient
        self.t_VFPClient = t_VFPClient

        # Message-Passing
        self.t_MP = t_MP
        self.cell_len = cell_len
        self.alpha_VFPClient = alpha_VFPClient
        self.alpha_VF = alpha_VF
        self.alpha_len = alpha_len
        self.alpha_MP = alpha_MP

        # VFP
        self.u_VFP = u_VFP
        self.t_VFP = t_VFP

        # PCIe
        self.N_VF = N_VF
        self.N_PAC = N_PAC
        self.u_PCIe = u_PCIe
        self.K = K

        # AFU
        self.u_AFU = u_AFU
        self.t_AFU = t_AFU
        self.P = P

        # RS
        self.u_RS = u_RS

    def U_HDFS(self):
        """HDFS throughput."""
        if self.u_coder is not None:
            return self.u_coder * self.N_coder
        elif self.t_coder is not None and self.cell_len is not None:
            return (self.t_coder / self.cell_len) * self.N_coder
        else:
            raise ValueError("Insufficient parameters for U_HDFS")

    def U_VFPClient(self):
        """VFPClient throughput."""
        if self.u_VFPClient is not None:
            return self.u_VFPClient * self.N_VFPClient
        elif self.t_VFPClient is not None and self.cell_len is not None:
            return (self.t_VFPClient / self.cell_len) * self.N_VFPClient
        else:
            raise ValueError("Insufficient parameters for U_VFPClient")

    def U_MP(self):
        """Message-Passing throughput."""
        if self.t_MP is not None and self.cell_len is not None:
            return self.t_MP / self.cell_len
        elif self.alpha_MP is not None:
            return self.alpha_MP * (self.N_VFPClient / self.N_VF)
        elif all(v is not None for v in [self.alpha_VFPClient, self.N_VFPClient, self.alpha_VF, self.N_VF, self.alpha_len, self.cell_len]):
            return (1 / self.cell_len) * (self.alpha_VFPClient * self.N_VFPClient) / (self.alpha_VF * self.N_VF) * (self.alpha_len * self.cell_len)
        else:
            raise ValueError("Insufficient parameters for U_MP")

    def U_VFP(self):
        """VFP throughput."""
        if self.u_VFP is not None:
            return self.u_VFP * self.N_VF
        elif self.t_VFP is not None and self.cell_len is not None:
            return (self.t_VFP / self.cell_len) * self.N_VF
        else:
            raise ValueError("Insufficient parameters for U_VFP")

    def U_PCIe(self):
        """PCIe throughput."""
        return self.N_PAC * (self.u_PCIe / self.K)

    def U_AFU(self):
        """AFU throughput."""
        if self.u_AFU is not None:
            return self.u_AFU * self.N_VF
        elif self.t_AFU is not None and self.cell_len is not None and self.P is not None:
            return (self.t_AFU / self.cell_len) * self.P * self.N_VF
        else:
            raise ValueError("Insufficient parameters for U_AFU")

    def U_RS(self):
        """RS IP throughput."""
        return self.u_RS * self.N_VF

    def get_all_throughputs(self):
        """Compute all throughput values and return as a dictionary."""
        throughputs = {}
        for method in [self.U_HDFS, self.U_VFPClient, self.U_MP, self.U_VFP, self.U_PCIe, self.U_AFU, self.U_RS]:
            name = method.__name__
            try:
                throughputs[name] = method()
            except ValueError:
                throughputs[name] = None
        return throughputs
