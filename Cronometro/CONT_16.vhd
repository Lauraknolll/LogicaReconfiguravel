library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity CONT_16 is 
	port(
		CLK : in std_logic;
		RST, EN, CLR : in std_logic;
		--LOAD : in std_logic;
		--DATA : in std_logic_vector (3 downto 0);
		Q : out std_logic_vector (3 downto 0)
	);
end entity;

architecture a_CONT_16 of CONT_16 is 

	signal Q_aux : unsigned (3 downto 0) := (others => '0');
	
	begin
	
		process(CLK, RST)
		begin
		
			if RST = '1' then                    -- coloca no estado inicial
				Q_aux <= (others => '0');
			
			elsif rising_edge(CLK) then
			
				if CLR = '1' then                 -- limpar tudo
					Q_aux <= (others => '0');
					
				elsif EN = '1' then
					
					--if LOAD = '1' then
						--Q_aux <= unsigned(DATA);
						
					--else
						Q_aux <= Q_aux + 1;
						
					--end if;
					
				end if;
				
			end if;
			
		end process;
		
		Q <= std_logic_vector(Q_aux);
		
end architecture a_CONT_16;