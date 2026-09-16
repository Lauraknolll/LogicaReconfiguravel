library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity CRONOMETRO is
	port(
		CLK : in std_logic;
		BOTAO_PAUSAR, BOTAO_RESETAR : in std_logic;
		SEGUNDOS, CENTESIMOS : out std_logic_vector (7 downto 0);
		CENTESIMOS_UNIDADE_7SEG, CENTESIMOS_DEZENA_7SEG : out STD_LOGIC_VECTOR(6 downto 0);
		SEGUNDOS_UNIDADE_7SEG, SEGUNDOS_DEZENA_7SEG : out STD_LOGIC_VECTOR(6 downto 0);
		EN : out std_logic
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
	
	component DIVISOR_10ms is
		port(
			CLK_50MHz : in std_logic;
			CLK_10ms  : out std_logic
		);
	end component;

	component bcd_para_7seg is
     	port (
        BCD   : in STD_LOGIC_VECTOR(3 downto 0);
        SEG : out STD_LOGIC_VECTOR(6 downto 0)             
    	);
    end component;
	
	signal clock, enable : std_logic;
	
	signal saida_unidade_cent, saida_dezena_cent, saida_unidade_seg, saida_dezena_seg: std_logic_vector (3 downto 0) := (others => '0');
	signal saida_unidade_cent_7seg, saida_dezena_cent_7seg, saida_unidade_segundos_7seg, saida_dezena_segundos_7seg: std_logic_vector (6 downto 0) := (others => '0');
	signal clear_dezena_seg, clear_unidade_seg, clear_dezena_cent, clear_unidade_cent : std_logic := '0';
	signal enable_dezena_seg, enable_unidade_seg, enable_dezena_cent, enable_unidade_cent : std_logic := '0';
	signal detecta5_dezena_seg, detecta9_unidade_seg, detecta9_dezena_cent, detecta9_unidade_cent : std_logic := '0';
	signal clock_enable : std_logic := '0';
	signal pausado : std_logic := '1';
	signal reset : std_logic := '0';
	signal botao_pausar_neg, botao_resetar_neg : std_logic;
	signal botao_pausar_neg_anterior : std_logic := '0';
	
	begin
	
	clock  <= CLK;
	botao_pausar_neg <= (NOT BOTAO_PAUSAR);
	botao_resetar_neg <= (NOT BOTAO_RESETAR);
	
	divisor : DIVISOR_10ms port map (CLK_50MHz => clock, CLK_10ms => clock_enable);
	
	detecta9_unidade_cent <= saida_unidade_cent(3)     AND (NOT saida_unidade_cent(2)) AND (NOT saida_unidade_cent(1)) AND saida_unidade_cent(0);
	detecta9_dezena_cent  <= saida_dezena_cent(3)      AND (NOT saida_dezena_cent(2))  AND (NOT saida_dezena_cent(1))  AND saida_dezena_cent(0);
	detecta9_unidade_seg  <= saida_unidade_seg(3)      AND (NOT saida_unidade_seg(2))  AND (NOT saida_unidade_seg(1))  AND saida_unidade_seg(0);
	detecta5_dezena_seg   <= (NOT saida_dezena_seg(3)) AND saida_dezena_seg(2)         AND (NOT saida_dezena_seg(1))   AND saida_dezena_seg(0);

	enable_unidade_cent <= clock_enable AND (NOT pausado);
	enable_dezena_cent  <= clock_enable AND detecta9_unidade_cent  AND (NOT pausado);
	enable_unidade_seg  <= clock_enable AND detecta9_dezena_cent AND detecta9_unidade_cent  AND (NOT pausado);
	enable_dezena_seg   <= clock_enable AND detecta9_unidade_seg AND detecta9_dezena_cent AND detecta9_unidade_cent  AND (NOT pausado);
	
	
	clear_unidade_cent <= detecta9_unidade_cent;
	clear_dezena_cent  <= detecta9_dezena_cent AND detecta9_unidade_cent;
	clear_unidade_seg  <= detecta9_unidade_seg AND detecta9_dezena_cent AND detecta9_unidade_cent;
	clear_dezena_seg   <= detecta5_dezena_seg  AND detecta9_unidade_seg AND detecta9_dezena_cent AND detecta9_unidade_cent;
	
	dezena_segundo    : CONT_16 port map (CLK => clock, RST => reset, EN => enable_dezena_seg,   CLR => clear_dezena_seg,   Q => saida_dezena_seg);
	unidade_segundo   : CONT_16 port map (CLK => clock, RST => reset, EN => enable_unidade_seg,  CLR => clear_unidade_seg,  Q => saida_unidade_seg);
	dezena_centesimo  : CONT_16 port map (CLK => clock, RST => reset, EN => enable_dezena_cent,  CLR => clear_dezena_cent,  Q => saida_dezena_cent);
	unidade_centesimo : CONT_16 port map (CLK => clock, RST => reset, EN => enable_unidade_cent, CLR => clear_unidade_cent, Q => saida_unidade_cent);
	
	dezena_segundo_7seg   : bcd_para_7seg port map (bcd => saida_dezena_seg, seg => saida_dezena_segundos_7seg );
	unidade_segundo_7seg  : bcd_para_7seg port map (bcd => saida_unidade_seg, seg => saida_unidade_segundos_7seg );
	dezena_centesimo_7seg : bcd_para_7seg port map (bcd => saida_dezena_cent, seg => saida_dezena_cent_7seg );
	unidade_centesimo_7seg: bcd_para_7seg port map (bcd =>saida_unidade_cent, seg => saida_unidade_cent_7seg );

	SEGUNDOS   <= saida_dezena_seg  & saida_unidade_seg;
	CENTESIMOS <= saida_dezena_cent & saida_unidade_cent;

	SEGUNDOS_DEZENA_7SEG <= saida_dezena_segundos_7seg;
	SEGUNDOS_UNIDADE_7SEG <= saida_unidade_segundos_7seg;

	CENTESIMOS_DEZENA_7SEG <= saida_dezena_cent_7seg;
	CENTESIMOS_UNIDADE_7SEG <=  saida_unidade_cent_7seg;
	
	EN <= clock_enable;
	
	process(CLK)
	begin
		
		if rising_edge(CLK) then
		
			if botao_pausar_neg = '1' AND botao_pausar_neg_anterior = '0' then
				pausado <= (NOT pausado);
			end if;
			botao_pausar_neg_anterior <= botao_pausar_neg;
			
			if pausado = '1' then
				if botao_resetar_neg = '1' then
					reset <= '1';
				end if;
			else 
				reset <= '0'; 
			end if; 
			
		end if;
	
	end process;
	
end architecture a_CRONOMETRO;