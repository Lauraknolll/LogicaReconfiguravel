library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity CRONOMETRO_tb is
end entity;

architecture a_CRONOMETRO_tb of CRONOMETRO_tb is

	component CRONOMETRO is 
		port(
			CLK : in std_logic;
			BOTAO_PAUSAR, BOTAO_RESETAR : in std_logic;
			SEGUNDOS, CENTESIMOS : out std_logic_vector (7 downto 0);
			CENTESIMOS_UNIDADE_7SEG, CENTESIMOS_DEZENA_7SEG : out STD_LOGIC_VECTOR(6 downto 0);
			SEGUNDOS_UNIDADE_7SEG, SEGUNDOS_DEZENA_7SEG : out STD_LOGIC_VECTOR(6 downto 0);
			EN : out std_logic
		);
	end component;
	
	signal clk, en  : std_logic := '0';
	signal s, c : std_logic_vector (7 downto 0);
	signal bot_p : std_logic := '0';
	signal bot_r : std_logic := '1';
	signal a, b, ca, d : std_logic_vector (6 downto 0);
	
	begin 
		uut : CRONOMETRO port map(EN => en, CLK => clk, BOTAO_PAUSAR => bot_p, BOTAO_RESETAR => bot_r, SEGUNDOS => s, CENTESIMOS => c, CENTESIMOS_UNIDADE_7SEG => a, CENTESIMOS_DEZENA_7SEG => b, SEGUNDOS_UNIDADE_7SEG => ca, SEGUNDOS_DEZENA_7SEG => d);
	
	
	clk_proc : process
		begin
			clk <= '0';
			wait for 10 ns;
			clk <= '1';
			wait for 10 ns;
			
	end process;
	
	main_proc : process
		begin
			wait for 15 ns;
		   bot_p	<= '1';
			wait for 310 ns;
			bot_p <= '0';
			wait for 15 ns;
		   bot_p	<= '1';
			wait for 1066 ns;
			bot_p <= '0';
			bot_r <= '0';
			wait for 15 ns;
			bot_p <= '1';
			wait;
	end process;
	
end architecture a_CRONOMETRO_tb;