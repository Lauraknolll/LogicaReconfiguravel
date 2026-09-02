library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity CRONOMETRO is
	port(
		CLK : in std_logic;
		EN : in std_logic;
		SEGUNDOS, CENTESIMOS : out std_logic_vector (7 downto 0)
	);
end entity;

architecture a_CRONOMETRO of CRONOMETRO is

	component CONT_16 is 
		port(
			CLK : in std_logic;
			RST, EN, CLR : in std_logic;
			Q : out std_logic_vector (3 downto 0)
		);
	end component;
	
	signal clock, enable : std_logic;
	
	signal saida_unidade_cent, saida_dezena_cent, saida_unidade_seg, saida_dezena_seg: std_logic_vector (3 downto 0) := (others => '0');
	signal clear_dezena_seg, clear_unidade_seg, clear_dezena_cent, clear_unidade_cent : std_logic := '0';
	signal enable_dezena_seg, enable_unidade_seg, enable_dezena_cent : std_logic := '0';
	signal detecta5_dezena_seg, detecta9_unidade_seg, detecta9_dezena_cent, detecta9_unidade_cent : std_logic := '0';
	
	begin
	
	clock  <= CLK;
	enable <= EN;
	
	detecta9_unidade_cent <= saida_unidade_cent(3)     AND (NOT saida_unidade_cent(2)) AND (NOT saida_unidade_cent(1)) AND saida_unidade_cent(0);
	detecta9_dezena_cent  <= saida_dezena_cent(3)      AND (NOT saida_dezena_cent(2))  AND (NOT saida_dezena_cent(1))  AND saida_dezena_cent(0);
	detecta9_unidade_seg  <= saida_unidade_seg(3)      AND (NOT saida_unidade_seg(2))  AND (NOT saida_unidade_seg(1))  AND saida_unidade_seg(0);
	detecta5_dezena_seg   <= (NOT saida_dezena_seg(3)) AND saida_dezena_seg(2)         AND (NOT saida_dezena_seg(1))   AND saida_dezena_seg(0);

	enable_dezena_cent <= detecta9_unidade_cent;
	enable_unidade_seg <= detecta9_dezena_cent AND detecta9_unidade_cent;
	enable_dezena_seg  <= detecta9_unidade_seg AND detecta9_dezena_cent AND detecta9_unidade_cent;
	
	clear_unidade_cent <= detecta9_unidade_cent;
	clear_dezena_cent  <= detecta9_dezena_cent AND detecta9_unidade_cent;
	clear_unidade_seg  <= detecta9_unidade_seg AND detecta9_dezena_cent AND detecta9_unidade_cent;
	clear_dezena_seg   <= detecta5_dezena_seg  AND detecta9_unidade_seg AND detecta9_dezena_cent AND detecta9_unidade_cent;
	
	dezena_segundo    : CONT_16 port map (CLK => clock, RST => '0', EN => enable_dezena_seg,  CLR => clear_dezena_seg,   Q => saida_dezena_seg);
	unidade_segundo   : CONT_16 port map (CLK => clock, RST => '0', EN => enable_unidade_seg, CLR => clear_unidade_seg,  Q => saida_unidade_seg);
	dezena_centesimo  : CONT_16 port map (CLK => clock, RST => '0', EN => enable_dezena_cent, CLR => clear_dezena_cent,  Q => saida_dezena_cent);
	unidade_centesimo : CONT_16 port map (CLK => clock, RST => '0', EN => enable,             CLR => clear_unidade_cent, Q => saida_unidade_cent);

	SEGUNDOS   <= saida_dezena_seg  & saida_unidade_seg;
	CENTESIMOS <= saida_dezena_cent & saida_unidade_cent;
	
end architecture a_CRONOMETRO;