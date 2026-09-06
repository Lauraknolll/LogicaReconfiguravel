#Limpa e cria a biblioteca
if {[file exists work]} {
    vdel -lib work -all
}
vlib work
vmap work work

#Compila os arquivos VHDL
vcom -93 -work work CONT_16.vhd
vcom -93 -work work DIVISOR_10ms.vhd
vcom -93 -work work BCD_PARA_7SEG.vhd
vcom -93 -work work CRONOMETRO.vhd
vcom -93 -work work CRONOMETRO_tb.vhd

#Chama o simulador
vsim -voptargs=+acc work.CRONOMETRO_tb

#Adiciona os sinais na janela Wave
add wave -radix binary /CRONOMETRO_tb/clk
add wave -radix hexadecimal /CRONOMETRO_tb/s
add wave -radix hexadecimal /CRONOMETRO_tb/c

#Executa a simulação
run 3000 ns

#Ajusta o zoom
wave zoomfull