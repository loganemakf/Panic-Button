// PANIC BUTTON LITHOPHANE CUSTOMIZATION PARAMETERS
PANIC_TEXT = "PANIC";
FONT = "DIN Alternate";
FONT_SIZE = 16.7;
MINIMUM_THICKNESS = 0.6;
CENTER_TWEAK = -0.7;     // added to text x-pos to make it look better centered

// CONSTANTS (tune to your printer if necessary)
_WIDTH = 70;
_HEIGHT = 29.8;
_TOTAL_DEPTH = 3.2;
_WINDOW_WIDTH = 66.5;
_WINDOW_HEIGHT = 27;
_FRAME_CUT_DEPTH = 1;


difference() {
    difference() {
        color("white", 1.0) cube([_WIDTH, _HEIGHT, _TOTAL_DEPTH]);

        // frame represents the outer (frontmost) edges of the window on the panic button shell
        color("red", 0.75) translate([0, 0, _TOTAL_DEPTH-1]) {
            difference() {
                // cube that will become the frame
                translate([-0.05, -0.05, 0]) {
                    cube([_WIDTH+0.1, _HEIGHT+0.1, _FRAME_CUT_DEPTH+0.1]);
                }
                
                // cube that represents the portion of the lithophane that
                // will sit flush with the front of the panic button
                translate([(_WIDTH - _WINDOW_WIDTH)/2, (_HEIGHT - _WINDOW_HEIGHT)/2, -0.1]) {
                    cube([_WINDOW_WIDTH, _WINDOW_HEIGHT, _FRAME_CUT_DEPTH+0.2]);
                }
            }
        }
    }

    // subtract the text
    color("lightgray", 1.0) translate([_WIDTH/2+CENTER_TWEAK, _HEIGHT/2, MINIMUM_THICKNESS]) {
        linear_extrude(height=(_TOTAL_DEPTH - MINIMUM_THICKNESS + 0.1)) {
            text(PANIC_TEXT, size=FONT_SIZE, font=FONT, halign="center", valign="center");
        }
    }
}