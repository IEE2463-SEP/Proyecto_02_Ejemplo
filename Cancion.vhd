library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.STD_LOGIC_ARITH.ALL;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
use IEEE.NUMERIC_STD.ALL;

entity Cancion is
Port(
clk: in std_logic;
rst: in std_logic;
frecuencia: out std_logic_vector(19 downto 0) :=x"00000"
);
--  Port ( );
end Cancion;

architecture Behavioral of Cancion is
--Extenderemos la memoria a 16 datos (igualmente de 4 bits)
type ram_array is array (0 to 15) of std_logic_vector (19 downto 0);
signal ram_data: ram_array := (
    x"4DD9E", -- G4
        x"455BB", -- A4
        x"4DD9E", -- G4
        x"57952", -- F4
        x"5CD44", -- A4
        x"6827D", -- A4
        x"57952", -- G4
        x"4DD9E", -- F4
        x"8AB76", -- F4
        x"7C0E2", -- E4
        x"74ECF", -- E4
        x"6827D", -- D4
        x"57952", -- D4
        x"4DD9E", -- C4
        x"8AB76", -- G4 (new note)
        x"8AB76" -- G4 (new note)
    );
-- Divisor de clock
    constant CLOCK_FREQUENCY : integer := 125000000; -- Frecuencia del reloj de entrada en Hz
    constant TARGET_TIME : real := 0.3; -- Tiempo objetivo en segundos
    constant DIVIDER_VALUE : integer := 37499999;
    
    signal counter : integer range 0 to DIVIDER_VALUE := 0;
    signal clk_out: std_logic;
    signal address: integer := 0;

begin
    process(clk, rst)
    begin
        if rst = '1' then
            counter <= 0;
            clk_out <= '0';
        elsif rising_edge(clk) then
            if counter = DIVIDER_VALUE then
                counter <= 0;
                clk_out <= not clk_out;
            else
                counter <= counter + 1;
            end if;
        end if;
    end process;
    

process(clk_out)
begin
if(rising_edge(clk_out)) then
            address <= address + 1;
            if address > 15 then
                address <= 0;
            end if;
            frecuencia <= ram_data(address);
    end if;
end process;



end Behavioral;
