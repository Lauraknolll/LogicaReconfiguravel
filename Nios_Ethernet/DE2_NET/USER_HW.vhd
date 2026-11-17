library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity USER_HW is 
	port(
		CLK, RST, CS : in std_logic;
		ESCREVER, LER : in std_logic;
		DADO_ESCRITA : in std_logic_vector (31 downto 0);
		DADO_LEITURA : out std_logic_vector (31 downto 0)
	);
end entity;

architecture a_USER_HW of USER_HW is 
	
	signal dado_escrita0, dado_leitura0 : std_logic_vector (31 downto 0);
	
	begin
		
		dado_escrita0 <= DADO_ESCRITA;
		
		main_proc: process(CLK, RST)

		begin
			if RST = '1' then
				dado_leitura0 <= (others => '0');
			
			elsif rising_edge(CLK) then
				
				if CS = '1' AND LER = '1' then
						dado_leitura0 <= std_logic_vector(unsigned(dado_escrita0) + 1);
				end if;
	
			end if;
		end process;
		
		DADO_LEITURA <= dado_leitura0;

end architecture a_USER_HW;