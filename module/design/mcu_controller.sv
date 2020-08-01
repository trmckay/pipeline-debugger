//////////////////////////////////////////////////////////////////////////////////
// Company: Cal Poly, SLO
// Engineer: Trevor McKay
// 
// Module Name: Otter General Purpose Controller (OGPC)
// Description: Hardware module to add support for remote debugging, programming,
// etc. via low-level control of a target MCU.
// 
// Revision: 0.20
//
// Revision  0.01 - File Created
// Revision  0.10 - Controller first rev.
// Revision  0.11 - Increase project scope
// Revision  0.20 - First rev. serial module, byte granularity
//
// TODO:
//   - serial decoder
//   - MCU integration
//   - testing
//   - write documentation
/////////////////////////////////////////////////////////////////////////////////

`timescale 1ns / 1ps

typedef enum logic [3:0] {
    // arguments: 0 bytes
    FN_NONE        = 4'h0,
    FN_PAUSE       = 4'h1,
    FN_RESUME      = 4'h2,
    FN_STEP        = 4'h3,
    FN_RESET       = 4'h4,
    FN_STATUS      = 4'h5, // reply

    // arguments: 4 bytes
    FN_MEM_RD_BYTE = 4'h6, // reply
    FN_MEM_RD_WORD = 4'h7, // reply
    FN_REG_RD      = 4'h8, // reply
    FN_BR_PT_ADD   = 4'h9,
    FN_BR_PT_RM    = 4'hA,

    // arguments: 4 bytes + 4 bytes
    FN_MEM_WR_BYTE = 4'hB, // recieves whole word for simplicity
    FN_MEM_WR_WORD = 4'hC,
    FN_REG_WR      = 4'hD
} DEBUG_FN;


/////////////////////////////////////////////////////////////////////////////////
// Module Name: Controller Wrapper
// Description: Link the serial decoder and controller FSM 
/////////////////////////////////////////////////////////////////////////////////

module mcu_controller(
    input clk,

    // user <-> debugger (via serial)
    input srx,
    output stx,

    // MCU -> debugger
    input [31:0] pc,
    input mcu_busy,
    input [31:0] d_rd,
    input error,

    // debugger -> MCU
    output [31:0] d_in,
    output [31:0] addr,
    output pause,
    output resume,
    output reset,
    output reg_rd,
    output reg_wr,
    output mem_rd,
    output mem_wr,
    output mem_rw_byte,
    output valid
);

    logic l_ctrlr_busy, l_serial_valid;
    DEBUG_FN l_debug_fn;
    logic [31:0] l_addr;

    assign addr = l_addr;

    serial serial(
        .clk(clk),
        .reset(1'b0),
        .srx(srx),
        .ctrlr_busy(l_ctrlr_busy),
        .d_rd(d_rd),
        .error(error),
        
        .stx(stx),
        .debug_fn(l_debug_fn),
        .addr(l_addr),
        .d_in(d_in),
        .out_valid(l_serial_valid)
    );
    
    controller_fsm fsm(
        .clk(clk),
        .debug_fn(l_debug_fn),
        .addr(l_addr),
        .in_valid(serial_valid),
        .pc(pc),
        .mcu_busy(mcu_busy),

        .pause(pause),
        .reset(reset),
        .resume(resume),
        .rf_rd(rf_rd),
        .rf_wr(rf_wr),
        .mem_rd(mem_rd),
        .mem_wr(mem_wr),
        .mem_rw_byte(mem_rw_byte),
        .out_valid(valid),
        .ctrlr_busy(l_ctrlr_busy)
    );
    
endmodule // module mcu_controller


/////////////////////////////////////////////////////////////////////////////////
// Module Name: Serial Decoder
// Description: Decode incoming UART commands and encode responses
/////////////////////////////////////////////////////////////////////////////////

// TODO: rewrite serial driver to support word communication

module serial(
    input clk,
    input reset,

    // user <-> serial
    input srx,
    output stx,

    // controller -> sdec
    input ctrlr_busy,

    // mcu -> sdec
    input [31:0] d_rd,
    input error,

    // sdec -> controller
    output DEBUG_FN debug_fn,
    output [31:0] addr,
    output [31:0] d_in,
    output out_valid
);

endmodule // module serial


/////////////////////////////////////////////////////////////////////////////////
// Module Name: Controller FSM
// Description: Issue relevent control signals for each debug function
/////////////////////////////////////////////////////////////////////////////////

module controller_fsm(
    // INPUTS
        input clk,

        // sdec -> controller
            input DEBUG_FN debug_fn,
            input logic [31:0] addr,
            input logic in_valid,
            
        // MCU -> controller
            input logic [31:0] pc,
            input logic mcu_busy,
    
    // OUTPUTS
        // controller -> MCU
            output logic pause,
            output logic reset,
            output logic resume,
            output logic out_valid,
            output logic rf_rd,
            output logic mem_rd,
            output logic rf_wr,
            output logic mem_wr,
            output logic mem_rw_byte,

        // controller -> sdec
            output logic ctrlr_busy
    );
    
    // keep track of mcu state
    reg r_mcu_paused = 0;
    logic l_mcu_paused_in;

    // breakpoints
    logic [31:0] break_pts[8];
    initial
        for (int i = 0; i < 8; i++)
            break_pts[i] = 'Z;
    reg [2:0] r_num_break_pts = 0;
    logic l_bp_add, l_hit;

    // states for controller
    typedef enum logic [3:0] {
        S_IDLE,
        S_WAIT_PAUSE,
        S_WAIT_RESUME,
        S_WAIT_MEM_RD,
        S_WAIT_MEM_WR,
        S_WAIT_REG_RD,
        S_WAIT_REG_WR,
        S_WAIT_STEP,
        S_BREAK_HIT
    } STATE;
    
    STATE r_ps = S_IDLE, l_ns; 
    
    always_ff @(posedge clk) begin
        // update paused state
        r_mcu_paused <= l_mcu_paused_in; 

        // compare pc to all breakpoints
        l_hit = 0;
        if (!r_mcu_paused) begin
            for (int i = 0; i < 8; i++) begin
                if ((pc + 4) == break_pts[i]) begin
                    r_ps <= S_BREAK_HIT;
                    l_hit = 1;
                end
            end
        end
        if (!l_hit) r_ps <= l_ns;
        
        // load in new breakpoints
        if (l_bp_add) begin
            break_pts[r_num_break_pts] <= addr;
            r_num_break_pts <= r_num_break_pts + 1;
        end
    end // always_ff @(posedge clk)
    
    always_comb begin
        pause = 0;
        reset = 0;
        resume = 0;
        l_bp_add= 0;
        rf_rd = 0;
        rf_wr = 0;
        mem_rd = 0;
        mem_wr = 0;
        mem_rw_byte = 0;
        l_ns  = S_IDLE;

        /* controller will output no commands and
        * accept no input in its default state */
        
        // output is invalid unless specified
        out_valid = 0;
        // busy unless specified
        ctrlr_busy = 1;

        // keep these values by default
        l_mcu_paused_in = r_mcu_paused;

        case(r_ps)

            S_IDLE: begin
                // check for valid from sdec high
                if (in_valid) begin
                    case(debug_fn)
                        FN_PAUSE: begin
                            pause = 1;
                            out_valid = 1;
                            l_ns = S_WAIT_PAUSE;
                        end
                        FN_RESUME: begin
                            resume = 1;
                            out_valid = 1;
                            l_ns = S_WAIT_RESUME;
                        end
                        FN_STEP: begin
                            // step only supported if MCU is paused
                            if (r_mcu_paused) begin
                                resume = 1;
                                out_valid = 1;
                                l_ns = S_WAIT_STEP;
                               end
                            else begin
                                ctrlr_busy = 0;
                                l_ns = S_IDLE;
                            end
                        end
                        FN_BR_PT_ADD: begin
                            ctrlr_busy = 0;
                            l_bp_add = 1;
                            l_ns = S_IDLE;
                        end
                        FN_MEM_RD_WORD: begin
                            mem_rd = 1;
                            l_ns = S_WAIT_MEM_RD;
                        end
                        FN_MEM_WR_WORD: begin
                            mem_wr = 1;
                            l_ns = S_WAIT_MEM_WR;
                        end
                        FN_MEM_RD_BYTE: begin
                            mem_rd = 1;
                            mem_rw_byte = 1;
                            l_ns = S_WAIT_MEM_RD;
                        end
                        FN_MEM_WR_BYTE: begin
                            mem_wr = 1;
                            mem_rw_byte = 1;
                            l_ns = S_WAIT_MEM_WR;
                        end

                    endcase // case(debug_fn)
                end            
                // no command given, stay idle
                else begin
                    ctrlr_busy = 0;
                    l_ns = S_IDLE;
                end
            end
            
            S_WAIT_PAUSE: begin
                if (mcu_busy) begin
                    out_valid = 1;
                    pause = 1;
                    l_ns = S_WAIT_PAUSE;
                end
                else begin
                    l_mcu_paused_in = 1;
                    ctrlr_busy = 0;
                    l_ns = S_IDLE;
                end
            end

            S_WAIT_RESUME: begin
                if (mcu_busy) begin
                    resume = 1;
                    out_valid = 1;
                    l_ns = S_WAIT_RESUME;
                end
                else begin
                    l_mcu_paused_in = 0;
                    ctrlr_busy = 0;
                    l_ns = S_IDLE;
                end
            end

            S_WAIT_STEP: begin
                out_valid = 1;
                // wait for resume
                if (mcu_busy) begin
                    resume = 1;
                    l_ns = S_WAIT_STEP;
                end
                // pause on next cycle
                else begin
                    pause = 1;
                    l_ns = S_WAIT_PAUSE;
                end
            end

            S_WAIT_MEM_RD: begin
                if (mcu_busy) begin
                    out_valid = 1;
                    mem_rd = 1;
                    l_ns = S_WAIT_MEM_RD;
                end
                else begin
                    ctrlr_busy = 0;
                    l_ns = S_IDLE;
                end
            end

            S_WAIT_MEM_WR: begin
                if (mcu_busy) begin
                    out_valid = 1;
                    mem_wr = 1;
                    l_ns = S_WAIT_MEM_RD;
                end
                else begin
                    ctrlr_busy = 0;
                    l_ns = S_IDLE;
                end
            end

            S_WAIT_REG_RD: begin
                if (mcu_busy) begin
                    out_valid = 1;
                    rf_rd = 1;
                    l_ns = S_WAIT_REG_RD;
                end
                else begin
                    ctrlr_busy = 0;
                    l_ns = S_IDLE;
                end
            end

            S_WAIT_REG_WR: begin
                if (mcu_busy) begin
                    out_valid = 1;
                    rf_wr = 1;
                    l_ns = S_WAIT_REG_WR;
                end
                else begin
                    ctrlr_busy = 0;
                    l_ns = S_IDLE;
                end
            end

            S_BREAK_HIT: begin
                pause = 1;
                out_valid = 1;
                l_ns = S_WAIT_PAUSE;
            end

            default: l_ns = S_IDLE;
        endcase // case(r_ps)

    end // always_comb

endmodule // module controller_fsm
