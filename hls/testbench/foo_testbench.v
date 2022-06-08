`timescale 1us/100ns

module foo_testbench;

    reg clk;
    reg reset;

    reg start;
    wire finish;

    reg [7:0] arg_0;
    reg [7:0] arg_1;
    reg [7:0] arg_2;
    reg [7:0] arg_3;
    reg [7:0] arg_4;
    reg [15:0] arg_5;
    wire [15:0] return_val;

    foo foo_dut (
        .clk(clk),
        .reset(reset),

        .start(start),
        .finish(finish),

        .arg_0(arg_0),
        .arg_1(arg_1),
        .arg_2(arg_2),
        .arg_3(arg_3),
        .arg_4(arg_4),
        .arg_5(arg_5),

        .return_val(return_val)
    );

    initial begin
        $dumpvars();

        $monitor(
            "time: %0t\treturn value: %d\tfinish: %b",
            $time,
            return_val,
            finish
        );

        clk = 1'b0;
        reset = 1'b0;
        start = 1'b0;

        arg_0 = 8'd5;   // a
        arg_1 = 8'd12;  // b
        arg_2 = 8'd4;   // c
        arg_3 = 8'd2;   // d
        arg_4 = 8'd9;   // e
        arg_5 = 16'd10; // f

        #10 reset = 1'b1;
        #10 clk = !clk;
        #10 clk = !clk;
        reset = 1'b0;
        #10 reset = 1'b0;
        #10 start = 1'b1;
        #10 clk = !clk;
        #10 clk = !clk;
        start = 1'b0;

        for (integer i = 0; i < 20; i = i + 1) begin
            #10 clk = !clk;
        end

        #25 $finish;
    end
    
endmodule
