# Copyright (C) 2020 Intel Corporation.
# SPDX-License-Identifier: MIT

#
# This file contains PR specific Quartus assignments
#------------------------------------

if { [info exist env(OFS_BUILD_TAG_FLAT) ] } { 
    post_message "Compiling Flat design..." 
} else {


    if { [info exist env(OFS_BUILD_TAG_PR_FLOORPLAN) ] } {
        set fp_tcl_file_name  [exec basename $env(OFS_BUILD_TAG_PR_FLOORPLAN)]
        post_message "Compiling User Specified PR Base floorplan $fp_tcl_file_name"
    
        if { [file exists $::env(BUILD_ROOT_REL)/syn/user_settings/$fp_tcl_file_name] == 0} {
            post_message "Warning User PR floorplan not found = /syn/user_settings/$fp_tcl_file_name"
        }
        
        set_global_assignment -name SOURCE_TCL_SCRIPT_FILE $::env(BUILD_ROOT_REL)/syn/user_settings/$fp_tcl_file_name
         
    } else {
        post_message "Compiling PR Base revision..." 
        #-------------------------------
        # Specify PR Partition and turn PR ON for that partition
        #-------------------------------
        set_global_assignment -name REVISION_TYPE PR_BASE
        
        #####################################################
        # Main PR Partition -- green_region
        #####################################################
        set_instance_assignment -name PARTITION green_region -to afu_top|port_gasket|pr_slot|afu_main
        set_instance_assignment -name CORE_ONLY_PLACE_REGION ON -to afu_top|port_gasket|pr_slot|afu_main
        set_instance_assignment -name RESERVE_PLACE_REGION ON -to afu_top|port_gasket|pr_slot|afu_main
        set_instance_assignment -name PARTIAL_RECONFIGURATION_PARTITION ON -to afu_top|port_gasket|pr_slot|afu_main


        # Place in these regions
        # set_instance_assignment -name PLACE_REGION "X50 Y65 X289 Y189" -to afu_top|port_gasket|pr_slot|afu_main
        # NOTE: use multiple regions
        #   X11_Y4 + (201,204)
        #   X212_Y60 + (70,136)
        set_instance_assignment -name PLACE_REGION "X11 Y4 X211 Y207;X212 Y60 X281 Y194" -to  afu_top|port_gasket|pr_slot|afu_main
        # Route in the whole device
        set_instance_assignment -name ROUTE_REGION "X0 Y0 X344 Y212" -to afu_top|port_gasket|pr_slot|afu_main


        set_instance_assignment -name CORE_ONLY_PLACE_REGION ON -to mem_ss_top|mem_ss_fm_inst|mem_ss_fm|intf_0
        # X234_Y0 + (110,40)
        set_instance_assignment -name PLACE_REGION "X234 Y0 X361 Y35" -to mem_ss_top|mem_ss_fm_inst|mem_ss_fm|intf_0
        # Shrink this in height (40 -> 20)
        # X234_Y0 + (110,20)
        # set_instance_assignment -name PLACE_REGION "X234 Y0 X361 Y19" -to mem_ss_top|mem_ss_fm_inst|mem_ss_fm|intf_0

        set_instance_assignment -name CORE_ONLY_PLACE_REGION ON -to mem_ss_top|mem_ss_fm_inst|mem_ss_fm|msa_0
        # Same here
        set_instance_assignment -name PLACE_REGION "X234 Y0 X361 Y35" -to mem_ss_top|mem_ss_fm_inst|mem_ss_fm|msa_0
        # set_instance_assignment -name PLACE_REGION "X234 Y0 X361 Y19" -to mem_ss_top|mem_ss_fm_inst|mem_ss_fm|msa_0

    }

}
