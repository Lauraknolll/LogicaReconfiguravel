library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity CRONOMETRO_tb is
end entity;

architecture a_CRONOMETRO_tb of CRONOMETRO_tb is

	component CRONOMETRO is 
		port(
			CLK : in std_logic;
			SEGUNDOS, CENTESIMOS : out std_logic_vector (7 downto 0)
		);
	end component;
	
	signal clk  : std_logic := '0';
	signal s, c : std_logic_vector (7 downto 0);
	
	begin 
		uut : CRONOMETRO port map(CLK => clk, SEGUNDOS => s, CENTESIMOS => c);
	
	
	clk_proc : process
		begin
			clk <= '0';
			wait for 10 ns;
			clk <= '1';
			wait for 10 ns;
			
	end process;
	
--	main_proc : process
--		begin
--		   en  <= '1';
--			wait;
--	end process;
	
end architecture a_CRONOMETRO_tb;