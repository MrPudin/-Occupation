/*
 * scad/indicator_case.scad
 * Case for Indicator
 * 
*/

//Global Constants
//Cube base for the foundation of drawing fancy edged cubes
module cube_base(width, length, height, diff=5)
{
    union()
    {
        //Center Space

        translate([width/2,length/2,height/2]) 
            cube([width- 2*diff+1, length- 2*diff+1, height], center=true);
        
        //Straight Edges
        translate([0,diff,0]) cube([diff, length - 2 * diff,height]);
        translate([width-diff,diff,0]) cube([diff, length - 2 * diff,height]);
        translate([diff,0,0]) cube([width-2*diff,diff,height]);
        translate([diff,length-diff,0]) cube([width-2*diff,diff,height]);
    }
}

//Draw a cube with rounded edges
module round_edge_cube(width, length, height, radius=5)
{
    union()
    {
        cube_base(width, length, height, radius);
        
        //Circles for the rounded corners
        for(x=[radius, width-radius])
        {
            for(y=[radius, length-radius])
            {
                translate([x,y,0]) cylinder(r=radius, h=height);
            }
        }
    }
}

//Draw a cube with trianglar edges
module triangle_edge_cube(width, length, height, tsize=5)
{
    union()
    {
        cube_base(width, length, height, tsize);
        
        //Triangles for Triangluar conrners
        for(x=[tsize, width-tsize])
        {
            for(y=[tsize, length-tsize])
            {
                translate([x,y,height/2]) rotate([0,0,45])
                    cube([tsize*1.4, tsize*1.4, height], center=true);
            }
        }
    }
}

module hexagon(radius, height)
{
    cylinder(r=radius, h=height, $fn=6);
}

//Attachment 
ATTACH_RADI = 2.5;
ATTACH_ENTRENCH = 5;
ATTACH_WIDTH = 10;
ATTACH_LENGTH= 15;

module attachment(width=8, length=16, height=3)
{
    //Attachment Hole Measurement
    translate([-ATTACH_WIDTH,length/2-ATTACH_LENGTH/2,0])  
    {
        
        difference()
        {
            cube([ATTACH_WIDTH, ATTACH_LENGTH, height]);
            translate([ATTACH_WIDTH-ATTACH_ENTRENCH, ATTACH_LENGTH/2, -1]) hexagon(ATTACH_RADI, height+2);
            translate([-3,0,-1]) rotate([0,0,-20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
            translate([0,ATTACH_LENGTH-2.8,-1]) rotate([0,0,20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
        }
    }

    translate([width+ATTACH_WIDTH,length/2+ATTACH_LENGTH/2,0]) rotate([0,0,180])
    {
        difference()
        {
            cube([ATTACH_WIDTH, ATTACH_LENGTH, height]);
            translate([ATTACH_WIDTH-ATTACH_ENTRENCH, ATTACH_LENGTH/2, -1]) hexagon(ATTACH_RADI, height+2);
            translate([-3,0,-1]) rotate([0,0,-20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
            translate([0,ATTACH_LENGTH-2.8,-1]) rotate([0,0,20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
        }
    }
}

module attachment_extrusion(width=8, length=16, height=3, extrude=10)
{
    //Attacment Extrusions
    translate([-ATTACH_WIDTH,length/2-ATTACH_LENGTH/2,0])  
    {
        difference()
        {
            union()
            {
                cube([ATTACH_WIDTH, ATTACH_LENGTH, height]);
                translate([ATTACH_WIDTH-ATTACH_ENTRENCH, ATTACH_LENGTH/2, 0]) 
                hexagon(ATTACH_RADI, height+extrude);
            }
            translate([-3,0,-1]) rotate([0,0,-20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
            translate([0,ATTACH_LENGTH-2.8,-1]) rotate([0,0,20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
        }
    }

    translate([width+ATTACH_WIDTH,length/2+ATTACH_LENGTH/2,0]) rotate([0,0,180])
    {
        difference()
        {
            union()
            {
                cube([ATTACH_WIDTH, ATTACH_LENGTH, height]);
                translate([ATTACH_WIDTH-ATTACH_ENTRENCH, ATTACH_LENGTH/2, 0]) 
                hexagon(ATTACH_RADI, height+extrude);
            }
            translate([-3,0,-1]) rotate([0,0,-20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
            translate([0,ATTACH_LENGTH-2.8,-1]) rotate([0,0,20]) cube([ATTACH_WIDTH+2, ATTACH_WIDTH * 0.4, height+2]);
        }
    }
}


module top_plate()
{
    length=51; width=51; height=2;

    difference()
    {
        display_width = 20;
        display_length = 20;
        display_height = height + 6;

        union()
        {
            hull()
            {
                triangle_edge_cube(width=width, length=length, height=height, tsize=7);
                translate([width/2-display_width/2, length/2-display_width/2, height+3]) 
                    round_edge_cube(width=display_width, length=display_length, height=1, radius=5); 
            }
            attachment(width=width, length=length, height=height);
        }
    
        //Display Hole
        translate([width/2-display_width/2, length/2-display_length/2, -1])
            round_edge_cube(width=display_width, length=display_length, height=display_height, radius=5); 
        
        //Button Holes
        button_length = 7;
        disp_button_distance = 7;
        translate([2,length/2 - button_length/2,-1]) 
            cube([button_length, button_length, height+5]);
        
        disp_button_distance = 7;
        translate([width- button_length- 2,length/2 - button_length/2,-1]) 
            cube([button_length, button_length, height+5]);
        
    }
}

module mid_plate()
{
    width=51; length=40; height=5;
    difference()
    {
        union()
        {
            triangle_edge_cube(width=width, length=length, height=height, tsize=6);
            translate([14,6.5,height]) cylinder(r=2,h=2);
            translate([width-14,6.5,height]) cylinder(r=2,h=2);

            attachment(width=width, length=length, height=height);
        }
        
        //Ports Hole
        translate([-1,length-8,-1]) cube([30+1,8+1,height+2]);
        
        //Battery Crevice
        translate([-0.5,0-1,-1]) cube([width+1,25+1,1+1]);
    }
}

module bottom_plate()
{
    width=51; length=55; height=5;
    
    difference()
    {
        union()
        {
            triangle_edge_cube(width=width, length=length, height=height, tsize=4);
            attachment_extrusion(width=width, length=length, height=height, extrude=20);
            
            //Corners of Battery Holder
            translate([-5,0,0]) cube([5,width/2,height]);
            translate([0,0,0]) cube([5,5,height]);
            translate([width,0,0]) cube([5,width/2,height]);
            translate([width-5,,0]) cube([5,5,height]);
        
            //Cable Management
            guide_height= 3;
            translate([0,0,height])
            {
                difference()
                {
                    triangle_edge_cube(width=width, length=length, height=guide_height, tsize=4);
                    translate([-0.05,-0.05,-0.05]) cube([width+0.1,length/2+0.1,guide_height+0.1]);
                    translate([2,0,0]) 
                        round_edge_cube(width=width-4, length=length-2, height=guide_height+1, radius=4);
                    translate([width/2-4, length-2-1,0]) cube([8,2+2,guide_height+1]);
                }
            }
        }
        //Battery Hole
        translate([-0.5,5-1,-1]) cube([width+1,25+1,height+10]);
    }
}

translate([15,0,0])
{
    //translate([0,0,0]) top_plate();
    //translate([80,0.0]) mid_plate();
    //translate([0,60,0]) bottom_plate();
}
translate([15,0,0]) top_plate();
