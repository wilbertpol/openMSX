namespace eval slot {

#
# get_selected_slot
#
set_help_text get_selected_slot \
{Returns the selected slot for the given memory page.
This proc is typically used as a helper for a larger proc.

 @param page The memory page (0-3)
 @result Returns a Tcl list with two elements.
         First element is the primary slot (0-3).
         Second element is the secondary slot (0-3) or 'X'
         in case this slot was not expanded
}
proc get_selected_slot { page } {
	set ps_reg [debug read "ioports" 0xA8]
	set ps [expr ($ps_reg >> (2 * $page)) & 0x03]
	if [machine_info "issubslotted" $ps] {
		set ss_reg [debug read "slotted memory" [expr 0x40000 * $ps + 0xFFFF]]
		set ss [expr (($ss_reg ^ 255) >> (2 * $page)) & 0x03]
	} else {
		set ss "X"
	}
	return [list $ps $ss]
}

#
# slotselect
#
set_help_text slotselect \
{Returns a nicely formatted overview of the selected slots.}
proc slotselect { } {
	set result ""
	for { set page 0 } { $page < 4 } { incr page } {
		foreach {ps ss} [get_selected_slot $page] {}
		append result [format "%04X: slot %d" [expr 0x4000 * $page] $ps]
		if {$ss != "X"} { append result "." $ss }
		append result "\n"
	}
	return $result
}

#
# get_mapper_size
#
set_help_text get_mapper_size \
{Returns the size of the memory mapper in a given slot.
Result is 0 when there is no memory mapper in the slot.}
proc get_mapper_size { ps ss } {
	set result 0
	catch {
		set device [machine_info slot $ps $ss 0]
		if { [debug desc $device] == "memory mapper" } {
			set result [expr [debug size $device] / 0x4000]
		}
	}
	return $result
}

#
# pc_in_slot
#
set_help_text pc_in_slot \
{Test whether the CPU's program counter is inside a certain slot.
Typically used to set breakpoints in specific slots.}
proc pc_in_slot { ps {ss "X"} {mapper "X"} } {
	set page [expr [reg PC] >> 14]
	foreach {pc_ps pc_ss} [get_selected_slot $page] {}
	if {($ps != "X") &&                    ($pc_ps != $ps)} { return false }
	if {($ss != "X") && ($pc_ss != "X") && ($pc_ss != $ss)} { return false }
	set mapper_size [get_mapper_size $pc_ps $pc_ss]
	if {($mapper_size == 0) || ($mapper == "X")} { return true }
	set pc_mapper [debug read "MapperIO" $page]
	return [expr $mapper == ($pc_mapper & ($mapper_size - 1))]
}

#
# slotmap
#
set_help_text slotmap \
{Gives an overview of the devices in the different slots.}
proc slotmap_helper { ps ss } {
	set result ""
	for { set page 0 } { $page < 4 } { incr page } {
		set name [machine_info slot $ps $ss $page]
		append result [format "%04X: %s\n" [expr $page * 0x4000] $name]
	}
	return $result
}
proc slotmap_name { ps ss } {
	set t [list $ps $ss]
	foreach slot [machine_info external_slot] {
		if {[string equal [lrange [machine_info external_slot $slot] 0 1] $t]} {
			return " (${slot})"
		}
	}
	return ""
}
proc slotmap { } {
	set result ""
	for { set ps 0 } { $ps < 4 } { incr ps } {
		if [machine_info issubslotted $ps] {
			for { set ss 0 } { $ss < 4 } { incr ss } {
				append result "slot $ps.$ss[slotmap_name $ps $ss]:\n"
				append result [slotmap_helper $ps $ss]
			}
		} else {
			append result "slot $ps[slotmap_name $ps X]:\n"
			append result [slotmap_helper $ps 0]
		}
	}
	return $result
}

#
# iomap
#
set_help_text iomap \
{Gives an overview of the devices connected to the different I/O ports.}
proc iomap_helper { prefix begin end name } {
	if [string equal $name "empty"] return ""
	set result [format "port %02X" $begin]
	if {$begin == ($end - 1)} {
		append result ":   "
	} else {
		append result [format "-%02X:" [expr $end - 1]]
	}
	append result " $prefix $name\n"
}
proc iomap {} {
	set result ""
	set port 0
	while {$port < 256} {
		set in  [machine_info input_port  $port]
		set out [machine_info output_port $port]
		set end [expr $port + 1]
		while { ($end < 256) &&
		        [string equal $in  [machine_info input_port  $end]] &&
		        [string equal $out [machine_info output_port $end]] } {
			incr end
		}
		if [string equal $in $out] {
			append result [iomap_helper "I/O" $port $end $in ]
		} else {
			append result [iomap_helper "I  " $port $end $in ]
			append result [iomap_helper "  O" $port $end $out]
		}
		set port $end
	}
	return $result
}

namespace export get_selected_slot
namespace export slotselect
namespace export get_mapper_size
namespace export pc_in_slot
namespace export slotmap
namespace export iomap

} ;# namespace slot

namespace import slot::*
