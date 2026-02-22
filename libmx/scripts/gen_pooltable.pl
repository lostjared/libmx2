#!/usr/bin/perl
# gen_pooltable.pl - Generate 3D pool table models in .mxmod format
#
# Usage:
#   perl gen_pooltable.pl --part=felt   > pooltable_felt.mxmod   # green playing surface
#   perl gen_pooltable.pl --part=wood   > pooltable_wood.mxmod   # rails, aprons, legs, diamonds
#   perl gen_pooltable.pl --part=pocket > pooltable_pocket.mxmod # black pocket holes
#   perl gen_pooltable.pl               > pooltable.mxmod        # all-in-one (default)
#
# The model is built at real-world scale matching vk_stickball dimensions:
#   Playing surface: 12.0 x 6.0 (X x Z), top at Y = 0
#   Rails:          0.3 thick, 0.3 tall (above surface)
#   Legs:           0.4 x 0.4, 2.0 tall (below surface)
#   Aprons:         connect the legs around the perimeter

use strict;
use warnings;
use POSIX qw(floor);
use Getopt::Long;

my $part = "all";
GetOptions("part=s" => \$part) or die "Usage: $0 [--part=felt|wood|pocket|all]\n";
$part = lc($part);
die "Unknown part: $part (use felt, wood, pocket, or all)\n"
    unless $part =~ /^(felt|wood|pocket|all)$/;

my $PI = 3.14159265358979323846;

# --- Table dimensions (matching vk_stickball) ---
my $TABLE_W    = 12.0;
my $TABLE_H    = 6.0;
my $HALF_W     = $TABLE_W / 2.0;
my $HALF_H     = $TABLE_H / 2.0;
my $BUMPER_W   = 0.30;       # rail/cushion width
my $RAIL_H     = 0.30;       # rail height above playing surface
my $BED_THICK  = 0.15;       # playing surface slab thickness
my $LEG_SIZE   = 0.40;       # leg cross-section (square)
my $LEG_H      = 2.00;       # leg height below table bed
my $APRON_H    = 0.50;       # apron height below table bed
my $POCKET_R   = 0.25;       # pocket radius (proportional to real table)
my $PKT_SEGS   = 24;         # segments for pocket circles

# Y coordinates
my $SURFACE_Y  = 0.0;                          # top of playing surface
my $BED_BOT_Y  = $SURFACE_Y - $BED_THICK;      # bottom of table bed
my $RAIL_TOP_Y = $SURFACE_Y + $RAIL_H;         # top of rails
my $LEG_BOT_Y  = $BED_BOT_Y - $LEG_H;          # bottom of legs
my $APRON_BOT_Y = $BED_BOT_Y - $APRON_H;       # bottom of aprons

# Pocket positions (X, Z) - corners and side centers, pulled inward onto felt
my @POCKET_POS = (
    [-$HALF_W + 0.25,  $HALF_H - 0.25],   # top-left
    [0.0,              $HALF_H - 0.15],    # top-center
    [ $HALF_W - 0.25,  $HALF_H - 0.25],   # top-right
    [-$HALF_W + 0.25, -$HALF_H + 0.25],   # bottom-left
    [0.0,             -$HALF_H + 0.15],    # bottom-center
    [ $HALF_W - 0.25, -$HALF_H + 0.25],   # bottom-right
);

# Accumulate geometry
my @verts;
my @texcoords;
my @normals;

# ============================================================
# Helper: add a quad (two triangles, CCW winding)
# p0-p1-p2-p3 in CCW order when viewed from normal direction
# ============================================================
sub add_quad {
    my ($p0, $p1, $p2, $p3, $n, $u0, $v0, $u1, $v1) = @_;
    $u0 //= 0.0;  $v0 //= 0.0;
    $u1 //= 1.0;  $v1 //= 1.0;

    # Triangle 1: p0, p1, p2
    push @verts, [@$p0], [@$p1], [@$p2];
    push @texcoords, [$u0,$v0], [$u1,$v0], [$u1,$v1];
    push @normals, [@$n], [@$n], [@$n];

    # Triangle 2: p2, p3, p0
    push @verts, [@$p2], [@$p3], [@$p0];
    push @texcoords, [$u1,$v1], [$u0,$v1], [$u0,$v0];
    push @normals, [@$n], [@$n], [@$n];
}

# ============================================================
# Helper: add a box given min corner (x0,y0,z0) and max (x1,y1,z1)
# ============================================================
sub add_box {
    my ($x0, $y0, $z0, $x1, $y1, $z1) = @_;

    # Front face (+Z)
    add_quad([$x0,$y0,$z1], [$x1,$y0,$z1], [$x1,$y1,$z1], [$x0,$y1,$z1],
             [0,0,1]);
    # Back face (-Z)
    add_quad([$x1,$y0,$z0], [$x0,$y0,$z0], [$x0,$y1,$z0], [$x1,$y1,$z0],
             [0,0,-1]);
    # Left face (-X)
    add_quad([$x0,$y0,$z0], [$x0,$y0,$z1], [$x0,$y1,$z1], [$x0,$y1,$z0],
             [-1,0,0]);
    # Right face (+X)
    add_quad([$x1,$y0,$z1], [$x1,$y0,$z0], [$x1,$y1,$z0], [$x1,$y1,$z1],
             [1,0,0]);
    # Bottom face (-Y)
    add_quad([$x0,$y0,$z0], [$x1,$y0,$z0], [$x1,$y0,$z1], [$x0,$y0,$z1],
             [0,-1,0]);
    # Top face (+Y)
    add_quad([$x0,$y1,$z1], [$x1,$y1,$z1], [$x1,$y1,$z0], [$x0,$y1,$z0],
             [0,1,0]);
}

# ============================================================
# Helper: add a cylinder (open-ended tube) along Y axis
# center at (cx, cy_base, cz), radius r, height h, facing outward
# ============================================================
sub add_cylinder {
    my ($cx, $cy_base, $cz, $r, $h, $segs, $inward) = @_;
    $inward //= 0;
    my $ny = $inward ? -1 : 1;

    for my $i (0 .. $segs - 1) {
        my $a0 = 2.0 * $PI * $i / $segs;
        my $a1 = 2.0 * $PI * ($i + 1) / $segs;

        my $x0 = $cx + $r * cos($a0);
        my $z0 = $cz + $r * sin($a0);
        my $x1 = $cx + $r * cos($a1);
        my $z1 = $cz + $r * sin($a1);

        my $nx0 = cos($a0) * ($inward ? -1 : 1);
        my $nz0 = sin($a0) * ($inward ? -1 : 1);
        my $nx1 = cos($a1) * ($inward ? -1 : 1);
        my $nz1 = sin($a1) * ($inward ? -1 : 1);

        my $y0 = $cy_base;
        my $y1 = $cy_base + $h;

        my $u0 = $i / $segs;
        my $u1 = ($i + 1) / $segs;

        if ($inward) {
            # Triangle 1
            push @verts, [$x1,$y0,$z1], [$x0,$y0,$z0], [$x0,$y1,$z0];
            push @texcoords, [$u1,0], [$u0,0], [$u0,1];
            push @normals, [$nx1,0,$nz1], [$nx0,0,$nz0], [$nx0,0,$nz0];
            # Triangle 2
            push @verts, [$x0,$y1,$z0], [$x1,$y1,$z1], [$x1,$y0,$z1];
            push @texcoords, [$u0,1], [$u1,1], [$u1,0];
            push @normals, [$nx0,0,$nz0], [$nx1,0,$nz1], [$nx1,0,$nz1];
        } else {
            # Triangle 1
            push @verts, [$x0,$y0,$z0], [$x1,$y0,$z1], [$x1,$y1,$z1];
            push @texcoords, [$u0,0], [$u1,0], [$u1,1];
            push @normals, [$nx0,0,$nz0], [$nx1,0,$nz1], [$nx1,0,$nz1];
            # Triangle 2
            push @verts, [$x1,$y1,$z1], [$x0,$y1,$z0], [$x0,$y0,$z0];
            push @texcoords, [$u1,1], [$u0,1], [$u0,0];
            push @normals, [$nx1,0,$nz1], [$nx0,0,$nz0], [$nx0,0,$nz0];
        }
    }
}

# ============================================================
# Helper: add a flat circle (disc) at given Y level
# ============================================================
sub add_disc {
    my ($cx, $cy, $cz, $r, $segs, $face_up) = @_;
    $face_up //= 1;
    my @n = $face_up ? (0, 1, 0) : (0, -1, 0);

    for my $i (0 .. $segs - 1) {
        my $a0 = 2.0 * $PI * $i / $segs;
        my $a1 = 2.0 * $PI * ($i + 1) / $segs;

        my $x0 = $cx + $r * cos($a0);
        my $z0 = $cz + $r * sin($a0);
        my $x1 = $cx + $r * cos($a1);
        my $z1 = $cz + $r * sin($a1);

        if ($face_up) {
            push @verts, [$cx,$cy,$cz], [$x0,$cy,$z0], [$x1,$cy,$z1];
        } else {
            push @verts, [$cx,$cy,$cz], [$x1,$cy,$z1], [$x0,$cy,$z0];
        }
        push @texcoords, [0.5,0.5], [0.5+0.5*cos($a0), 0.5+0.5*sin($a0)],
                                      [0.5+0.5*cos($a1), 0.5+0.5*sin($a1)];
        push @normals, [@n], [@n], [@n];
    }
}

# ============================================================
# Build the pool table geometry based on --part
# ============================================================

# --- FELT: TABLE BED (playing surface slab) ---
if ($part eq "felt" || $part eq "all") {
    add_box(-$HALF_W, $BED_BOT_Y, -$HALF_H,
             $HALF_W, $SURFACE_Y,  $HALF_H);
}

# --- WOOD: RAILS, APRONS, LEGS, DIAMONDS ---
if ($part eq "wood" || $part eq "all") {
    # Rails / Cushions (4 pieces around the perimeter)
    # Top rail (-Z side)
    add_box(-$HALF_W - $BUMPER_W, $SURFACE_Y, -$HALF_H - $BUMPER_W,
             $HALF_W + $BUMPER_W, $RAIL_TOP_Y, -$HALF_H);
    # Bottom rail (+Z side)
    add_box(-$HALF_W - $BUMPER_W, $SURFACE_Y,  $HALF_H,
             $HALF_W + $BUMPER_W, $RAIL_TOP_Y,  $HALF_H + $BUMPER_W);
    # Left rail (-X side)
    add_box(-$HALF_W - $BUMPER_W, $SURFACE_Y, -$HALF_H - $BUMPER_W,
            -$HALF_W,             $RAIL_TOP_Y,  $HALF_H + $BUMPER_W);
    # Right rail (+X side)
    add_box( $HALF_W,             $SURFACE_Y, -$HALF_H - $BUMPER_W,
             $HALF_W + $BUMPER_W, $RAIL_TOP_Y,  $HALF_H + $BUMPER_W);

    # Aprons (skirts below table bed, 4 sides)
    add_box(-$HALF_W, $APRON_BOT_Y,  $HALF_H,
             $HALF_W, $BED_BOT_Y,    $HALF_H + $BUMPER_W);
    add_box(-$HALF_W, $APRON_BOT_Y, -$HALF_H - $BUMPER_W,
             $HALF_W, $BED_BOT_Y,   -$HALF_H);
    add_box(-$HALF_W - $BUMPER_W, $APRON_BOT_Y, -$HALF_H,
            -$HALF_W,             $BED_BOT_Y,    $HALF_H);
    add_box( $HALF_W,             $APRON_BOT_Y, -$HALF_H,
             $HALF_W + $BUMPER_W, $BED_BOT_Y,    $HALF_H);

    # Table Legs (4 square legs at corners)
    my @leg_corners = (
        [-$HALF_W - $BUMPER_W/2, -$HALF_H - $BUMPER_W/2],
        [ $HALF_W + $BUMPER_W/2, -$HALF_H - $BUMPER_W/2],
        [-$HALF_W - $BUMPER_W/2,  $HALF_H + $BUMPER_W/2],
        [ $HALF_W + $BUMPER_W/2,  $HALF_H + $BUMPER_W/2],
    );
    my $lhs = $LEG_SIZE / 2.0;
    for my $leg (@leg_corners) {
        my ($lx, $lz) = @$leg;
        add_box($lx - $lhs, $LEG_BOT_Y, $lz - $lhs,
                $lx + $lhs, $BED_BOT_Y, $lz + $lhs);
    }

    # Diamond Markers on rails
    my $DIAM_SIZE = 0.06;
    my $DIAM_Y    = $RAIL_TOP_Y + 0.005;
    # Long rails: 7 markers
    for my $side (-1, 1) {
        my $z = $side * ($HALF_H + $BUMPER_W / 2.0);
        for my $j (1 .. 7) {
            my $frac = $j / 8.0;
            my $x = -$HALF_W + $frac * $TABLE_W;
            add_quad([$x, $DIAM_Y, $z - $DIAM_SIZE],
                     [$x + $DIAM_SIZE, $DIAM_Y, $z],
                     [$x, $DIAM_Y, $z + $DIAM_SIZE],
                     [$x - $DIAM_SIZE, $DIAM_Y, $z],
                     [0, 1, 0]);
        }
    }
    # Short rails: 3 markers
    for my $side (-1, 1) {
        my $x = $side * ($HALF_W + $BUMPER_W / 2.0);
        for my $j (1 .. 3) {
            my $frac = $j / 4.0;
            my $z = -$HALF_H + $frac * $TABLE_H;
            add_quad([$x - $DIAM_SIZE, $DIAM_Y, $z],
                     [$x, $DIAM_Y, $z - $DIAM_SIZE],
                     [$x + $DIAM_SIZE, $DIAM_Y, $z],
                     [$x, $DIAM_Y, $z + $DIAM_SIZE],
                     [0, 1, 0]);
        }
    }
}

# --- POCKET: 3D pocket holes with visible top disc, beveled rim, and inner depth ---
if ($part eq "pocket" || $part eq "all") {
    my $RIM_WIDTH    = 0.04;       # thin rim lip (proportional)
    my $RIM_HEIGHT   = 0.04;       # subtle raised rim
    my $OUTER_R      = $POCKET_R + $RIM_WIDTH;

    for my $pkt (@POCKET_POS) {
        my ($px, $pz) = @$pkt;

        # 1. Black disc sitting on felt surface (the visible pocket hole)
        add_disc($px, $SURFACE_Y + 0.006, $pz, $POCKET_R, $PKT_SEGS, 1);

        # 2. Beveled inner slope: from disc edge down into the hole
        #    This creates a funnel/bowl shape inside the pocket
        my $INNER_R   = $POCKET_R * 0.45;  # narrow bottom for deeper funnel look
        my $funnel_bot = $SURFACE_Y - 0.08; # deeper funnel for more visible 3D

        for my $i (0 .. $PKT_SEGS - 1) {
            my $a0 = 2.0 * $PI * $i / $PKT_SEGS;
            my $a1 = 2.0 * $PI * ($i + 1) / $PKT_SEGS;

            # Top edge (at pocket radius, surface level)
            my $tx0 = $px + $POCKET_R * cos($a0);
            my $tz0 = $pz + $POCKET_R * sin($a0);
            my $tx1 = $px + $POCKET_R * cos($a1);
            my $tz1 = $pz + $POCKET_R * sin($a1);

            # Bottom edge (smaller radius, below surface)
            my $bx0 = $px + $INNER_R * cos($a0);
            my $bz0 = $pz + $INNER_R * sin($a0);
            my $bx1 = $px + $INNER_R * cos($a1);
            my $bz1 = $pz + $INNER_R * sin($a1);

            # Normals point inward and down (into the funnel)
            my $nx0 = -cos($a0) * 0.6;
            my $nz0 = -sin($a0) * 0.6;
            my $nx1 = -cos($a1) * 0.6;
            my $nz1 = -sin($a1) * 0.6;
            my $ny = -0.8;

            push @verts, [$tx0,$SURFACE_Y+0.006,$tz0], [$tx1,$SURFACE_Y+0.006,$tz1], [$bx1,$funnel_bot,$bz1];
            push @texcoords, [0,0], [1,0], [1,1];
            push @normals, [$nx0,$ny,$nz0], [$nx1,$ny,$nz1], [$nx1,$ny,$nz1];

            push @verts, [$bx1,$funnel_bot,$bz1], [$bx0,$funnel_bot,$bz0], [$tx0,$SURFACE_Y+0.006,$tz0];
            push @texcoords, [1,1], [0,1], [0,0];
            push @normals, [$nx1,$ny,$nz1], [$nx0,$ny,$nz0], [$nx0,$ny,$nz0];
        }

        # Bottom disc of the funnel
        add_disc($px, $funnel_bot, $pz, $INNER_R, $PKT_SEGS, 0);

        # 3. Raised rim ring around the pocket (slopes up from felt to rim top)
        my $disc_y = $SURFACE_Y + 0.006;
        my $rim_y  = $SURFACE_Y + $RIM_HEIGHT;

        for my $i (0 .. $PKT_SEGS - 1) {
            my $a0 = 2.0 * $PI * $i / $PKT_SEGS;
            my $a1 = 2.0 * $PI * ($i + 1) / $PKT_SEGS;

            # Inner edge (at pocket radius, disc level)
            my $ix0 = $px + $POCKET_R * cos($a0);
            my $iz0 = $pz + $POCKET_R * sin($a0);
            my $ix1 = $px + $POCKET_R * cos($a1);
            my $iz1 = $pz + $POCKET_R * sin($a1);

            # Outer edge (at outer radius, raised rim level)
            my $ox0 = $px + $OUTER_R * cos($a0);
            my $oz0 = $pz + $OUTER_R * sin($a0);
            my $ox1 = $px + $OUTER_R * cos($a1);
            my $oz1 = $pz + $OUTER_R * sin($a1);

            # Bevel normal: points up and inward
            my $nx0 = -cos($a0) * 0.5;
            my $nz0 = -sin($a0) * 0.5;
            my $nx1 = -cos($a1) * 0.5;
            my $nz1 = -sin($a1) * 0.5;

            # Top surface of rim bevel
            push @verts, [$ix0,$disc_y,$iz0], [$ix1,$disc_y,$iz1], [$ox1,$rim_y,$oz1];
            push @texcoords, [0,0], [1,0], [1,1];
            push @normals, [$nx0,0.86,$nz0], [$nx1,0.86,$nz1], [$nx1,0.86,$nz1];

            push @verts, [$ox1,$rim_y,$oz1], [$ox0,$rim_y,$oz0], [$ix0,$disc_y,$iz0];
            push @texcoords, [1,1], [0,1], [0,0];
            push @normals, [$nx1,0.86,$nz1], [$nx0,0.86,$nz0], [$nx0,0.86,$nz0];
        }

        # 4. Outer wall of the rim (vertical face)
        for my $i (0 .. $PKT_SEGS - 1) {
            my $a0 = 2.0 * $PI * $i / $PKT_SEGS;
            my $a1 = 2.0 * $PI * ($i + 1) / $PKT_SEGS;

            my $ox0 = $px + $OUTER_R * cos($a0);
            my $oz0 = $pz + $OUTER_R * sin($a0);
            my $ox1 = $px + $OUTER_R * cos($a1);
            my $oz1 = $pz + $OUTER_R * sin($a1);

            my $nx0 = cos($a0);
            my $nz0 = sin($a0);
            my $nx1 = cos($a1);
            my $nz1 = sin($a1);

            push @verts, [$ox0,$disc_y,$oz0], [$ox1,$disc_y,$oz1], [$ox1,$rim_y,$oz1];
            push @texcoords, [0,0], [1,0], [1,1];
            push @normals, [$nx0,0,$nz0], [$nx1,0,$nz1], [$nx1,0,$nz1];

            push @verts, [$ox1,$rim_y,$oz1], [$ox0,$rim_y,$oz0], [$ox0,$disc_y,$oz0];
            push @texcoords, [1,1], [0,1], [0,0];
            push @normals, [$nx1,0,$nz1], [$nx0,0,$nz0], [$nx0,0,$nz0];
        }
    }
}

# ============================================================
# Output .mxmod format
# ============================================================
my $count = scalar @verts;

print "# Pool Table Model ($part) - generated by gen_pooltable.pl\n";
print "# Dimensions: ${TABLE_W} x ${TABLE_H} playing surface\n";
print "tri 0 0\n";
print "vert $count\n";
for my $v (@verts) {
    printf "%.4f %.4f %.4f\n", $v->[0], $v->[1], $v->[2];
}

print "\ntex $count\n";
for my $t (@texcoords) {
    printf "%.4f %.4f\n", $t->[0], $t->[1];
}

print "\nnorm $count\n";
for my $n (@normals) {
    printf "%.4f %.4f %.4f\n", $n->[0], $n->[1], $n->[2];
}
