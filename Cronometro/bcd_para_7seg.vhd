library ieee;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


entity BCD_PARA_7SEG is
    port (
        BCD   : in STD_LOGIC_VECTOR(3 downto 0);
        SEG : out STD_LOGIC_VECTOR(6 downto 0)  
    );
end entity;


architecture X of BCD_PARA_7SEG is



type tabela is array (15 downto 0) of STD_LOGIC_VECTOR (6 downto 0);




constant SETE_SEG : tabela := (
        0  => "1000000", -- '0'
        1  => "1111001", -- '1'
        2  => "0100100", -- '2'
        3  => "0110000", -- '3'
        4  => "0011001", -- '4'
        5  => "0010010", -- '5'
        6  => "0000010", -- '6'
        7  => "1111000", -- '7'
        8  => "0000000", -- '8'
        9  => "0010000", -- '9'
        10 => "0001000", -- 'A'
        11 => "0000011", -- 'b'
        12 => "1000110", -- 'C'
        13 => "0100011", -- 'd'
        14 => "0000110", -- 'E'
        15 => "0001110"  -- 'F'
    );


begin


seg <= SETE_SEG(to_integer(unsigned(bcd)));

    

end architecture;