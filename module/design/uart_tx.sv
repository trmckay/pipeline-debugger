`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////
//
// File Downloaded from http://www.nandland.com
//
//////////////////////////////////////////////////////////////////////
//
// Changes by Keefe Johnson:
//   Upgraded to SystemVerilog with minor changes.
//   Made counter size dynamic to handle low baud with fast clock.
//   NOTE: Not adding a reset, to avoid confusing other end.
//
// Changes by Trevor McKay:
//   Formatting and documentation.
//
//////////////////////////////////////////////////////////////////////
//
// This file contains the UART Transmitter.  This transmitter is able
// to transmit 8 bits of serial data, one start bit, one stop bit,
// and no parity bit.  When transmit is complete o_Tx_done will be
// driven high for one clock cycle.
//
// Set Parameter CLKS_PER_BIT as follows:
// CLKS_PER_BIT = (Frequency of i_Clock)/(Frequency of UART)
// Example: 50 MHz Clock, 115200 baud UART
// (50E6)/(115200) = 434.028 ~= 434
//
//////////////////////////////////////////////////////////////////////

module uart_tx #(parameter CLKS_PER_BIT = -1)(
    input        i_Clock,
    input        i_Tx_DV,
    input [7:0]  i_Tx_Byte,
    output       o_Tx_Active,
    output logic o_Tx_Serial,
    output       o_Tx_Done
);

    localparam CNT_SIZE = $clog2(CLKS_PER_BIT);  // can count up to CLKS_PER_BIT-1

    localparam s_IDLE         = 3'b000;
    localparam s_TX_START_BIT = 3'b001;
    localparam s_TX_DATA_BITS = 3'b010;
    localparam s_TX_STOP_BIT  = 3'b011;
    localparam s_CLEANUP      = 3'b100;

    logic [2:0]           r_SM_Main     = 0;
    logic [CNT_SIZE-1:0]  r_Clock_Count = 0;
    logic [2:0]           r_Bit_Index   = 0;
    logic [7:0]           r_Tx_Data     = 0;
    logic                 r_Tx_Done     = 0;
    logic                 r_Tx_Active   = 0;

    always_ff @(posedge i_Clock) begin

        case (r_SM_Main)

            s_IDLE: begin
                o_Tx_Serial   <= 1'b1;         // Drive Line High for Idle
                r_Tx_Done     <= 1'b0;
                r_Clock_Count <= 0;
                r_Bit_Index   <= 0;
                if (i_Tx_DV == 1'b1) begin
                    r_Tx_Active <= 1'b1;
                    r_Tx_Data   <= i_Tx_Byte;
                    r_SM_Main   <= s_TX_START_BIT;
                end
                else
                    r_SM_Main <= s_IDLE;
            end // s_IDLE


            // Send out Start Bit. Start bit = 0
            s_TX_START_BIT: begin
                o_Tx_Serial <= 1'b0;

                // Wait CLKS_PER_BIT-1 clock cycles for start bit to finish
                if (r_Clock_Count < CLKS_PER_BIT-1) begin
                    r_Clock_Count <= r_Clock_Count + 1;
                    r_SM_Main     <= s_TX_START_BIT;
                end
                else begin
                    r_Clock_Count <= 0;
                    r_SM_Main     <= s_TX_DATA_BITS;
                end
            end // s_TX_START_BIT


            // Wait CLKS_PER_BIT-1 clock cycles for data bits to finish
            s_TX_DATA_BITS: begin
                o_Tx_Serial <= r_Tx_Data[r_Bit_Index];

                if (r_Clock_Count < CLKS_PER_BIT-1) begin
                    r_Clock_Count <= r_Clock_Count + 1;
                    r_SM_Main     <= s_TX_DATA_BITS;
                end
                else begin
                    r_Clock_Count <= 0;

                    // Check if we have sent out all bits
                    if (r_Bit_Index < 7) begin
                        r_Bit_Index <= r_Bit_Index + 1;
                        r_SM_Main   <= s_TX_DATA_BITS;
                    end
                    else begin
                        r_Bit_Index <= 0;
                        r_SM_Main   <= s_TX_STOP_BIT;
                    end
                end
            end // s_TX_DATA_BITS


            // Send out Stop bit.  Stop bit = 1
            s_TX_STOP_BIT: begin
                o_Tx_Serial <= 1'b1;

                // Wait CLKS_PER_BIT-1 clock cycles for Stop bit to finish
                if (r_Clock_Count < CLKS_PER_BIT-1) begin
                    r_Clock_Count <= r_Clock_Count + 1;
                    r_SM_Main     <= s_TX_STOP_BIT;
                end
                else begin
                    r_Tx_Done     <= 1'b1;
                    r_Clock_Count <= 0;
                    r_SM_Main     <= s_CLEANUP;
                    r_Tx_Active   <= 1'b0;
                end
            end // s_Tx_STOP_BIT


            // Stay here 1 clock
            s_CLEANUP: begin
                r_Tx_Done <= 1'b1;
                r_SM_Main <= s_IDLE;
            end


            default: r_SM_Main <= s_IDLE;

        endcase // s_SM_Main
    end // always_ff @(posedge clk)

    assign o_Tx_Active = r_Tx_Active;
    assign o_Tx_Done   = r_Tx_Done;

endmodule // uart_tx
