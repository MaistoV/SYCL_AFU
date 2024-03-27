#create_clock -name i_clk -period 10 [get_ports {i_clk}]

# reset false path
set_false_path -from [get_ports {reset_button_n}] -to * 