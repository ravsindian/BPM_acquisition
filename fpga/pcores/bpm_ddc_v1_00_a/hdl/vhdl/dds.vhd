--------------------------------------------------------------------------------
--    This file is owned and controlled by Xilinx and must be used solely     --
--    for design, simulation, implementation and creation of design files     --
--    limited to Xilinx devices or technologies. Use with non-Xilinx          --
--    devices or technologies is expressly prohibited and immediately         --
--    terminates your license.                                                --
--                                                                            --
--    XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" SOLELY    --
--    FOR USE IN DEVELOPING PROGRAMS AND SOLUTIONS FOR XILINX DEVICES.  BY    --
--    PROVIDING THIS DESIGN, CODE, OR INFORMATION AS ONE POSSIBLE             --
--    IMPLEMENTATION OF THIS FEATURE, APPLICATION OR STANDARD, XILINX IS      --
--    MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION IS FREE FROM ANY      --
--    CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE FOR OBTAINING ANY       --
--    RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY       --
--    DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE   --
--    IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR          --
--    REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF         --
--    INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A   --
--    PARTICULAR PURPOSE.                                                     --
--                                                                            --
--    Xilinx products are not intended for use in life support appliances,    --
--    devices, or systems.  Use in such applications are expressly            --
--    prohibited.                                                             --
--                                                                            --
--    (c) Copyright 1995-2012 Xilinx, Inc.                                    --
--    All rights reserved.                                                    --
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- You must compile the wrapper file dds.vhd when simulating
-- the core, dds. When compiling the wrapper file, be sure to
-- reference the XilinxCoreLib VHDL simulation library. For detailed
-- instructions, please refer to the "CORE Generator Help".

-- The synthesis directives "translate_off/translate_on" specified
-- below are supported by Xilinx, Mentor Graphics and Synplicity
-- synthesis tools. Ensure they are correct for your synthesis tool(s).

LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
-- synthesis translate_off
LIBRARY XilinxCoreLib;
-- synthesis translate_on
ENTITY dds IS
  PORT (
    aclk : IN STD_LOGIC;
    aresetn : IN STD_LOGIC;
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tready : IN STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
  );
END dds;

ARCHITECTURE dds_a OF dds IS
-- synthesis translate_off
COMPONENT wrapped_dds
  PORT (
    aclk : IN STD_LOGIC;
    aresetn : IN STD_LOGIC;
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tready : IN STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
  );
END COMPONENT;

-- Configuration specification
  FOR ALL : wrapped_dds USE ENTITY XilinxCoreLib.dds_compiler_v5_0(behavioral)
    GENERIC MAP (
      c_accumulator_width => 16,
      c_amplitude => 0,
      c_chan_width => 1,
      c_channels => 1,
      c_debug_interface => 0,
      c_has_aclken => 0,
      c_has_aresetn => 1,
      c_has_channel_index => 0,
      c_has_m_data => 1,
      c_has_m_phase => 0,
      c_has_phase_out => 0,
      c_has_phasegen => 1,
      c_has_s_config => 0,
      c_has_s_phase => 0,
      c_has_sincos => 1,
      c_has_tlast => 0,
      c_has_tready => 1,
      c_latency => 8,
      c_m_data_has_tuser => 0,
      c_m_data_tdata_width => 32,
      c_m_data_tuser_width => 1,
      c_m_phase_has_tuser => 0,
      c_m_phase_tdata_width => 1,
      c_m_phase_tuser_width => 1,
      c_mem_type => 1,
      c_negative_cosine => 0,
      c_negative_sine => 1,
      c_noise_shaping => 0,
      c_optimise_goal => 0,
      c_output_width => 16,
      c_outputs_required => 2,
      c_phase_angle_width => 16,
      c_phase_increment => 2,
      c_phase_increment_value => "1111000000,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
      c_phase_offset => 0,
      c_phase_offset_value => "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0",
      c_por_mode => 0,
      c_s_config_sync_mode => 0,
      c_s_config_tdata_width => 1,
      c_s_phase_has_tuser => 0,
      c_s_phase_tdata_width => 1,
      c_s_phase_tuser_width => 1,
      c_use_dsp48 => 0,
      c_xdevicefamily => "virtex6"
    );
-- synthesis translate_on
BEGIN
-- synthesis translate_off
U0 : wrapped_dds
  PORT MAP (
    aclk => aclk,
    aresetn => aresetn,
    m_axis_data_tvalid => m_axis_data_tvalid,
    m_axis_data_tready => m_axis_data_tready,
    m_axis_data_tdata => m_axis_data_tdata
  );
-- synthesis translate_on

END dds_a;
