library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity DIVISOR_10ms is
	port(
		CLK_50MHz : in std_logic;
		CLK_10ms  : out std_logic
	);
end entity;

architecture a_DIVISOR_10ms of DIVISOR_10ms is

	signal contador : integer range 0 to 499_999 := 0;
	
	begin
	
	process(CLK_50MHz)
		begin
		
		if rising_edge(CLK_50MHz) then
		
			if contador = 499_999 then
				contador <= 0;
				CLK_10ms <= '1';
				
			else 
				contador <= contador + 1;
				CLK_10ms <= '0';
				
			end if;
			
		end if; 
		
	end process;
				
end architecture; 
