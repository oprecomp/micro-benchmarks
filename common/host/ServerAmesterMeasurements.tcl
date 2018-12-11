set ::bmcaddr delmonte-bmc
set ::bmcusr monitor
set ::bmcpw monitor
set ::port 9999

if {[llength $::script_argv] == 4} {
    set ::bmcaddr [lindex $::script_argv 0]
    set ::bmcusr [lindex $::script_argv 1]
    set ::bmcpw [lindex $::script_argv 2]
    set ::port [lindex $::script_argv 3]

} elseif {[llength $::script_argv] > 0} {
    puts "usage:  amester openpower.tcl <bmc_ip_addr> <userid> <password> <port>"
    puts "Example: amester ServerAmesterMeasurements.tcl delmonte-bmc monitor monitor 9999"
    exit 0
}

puts "Connecting to $::bmcaddr ..."
openpower mysys -addr $::bmcaddr -ipmi_user $::bmcusr -ipmi_passwd $::bmcpw
puts "Connection established"

#############################################################
### GLOBAL VARIABLES USED
#############################################################
# ::my_sensor_list 		short names of sensor 
# ::theameclist 		nodes available in the system
# ::allsensors			all sensor (each sensor on each node)
#
# ::acc_data 			holds values from start and stop (nested list)
#############################################################
puts "Getting the nodes:"
set ::theameclist [mysys get ameclist]
if {$::theameclist eq ""} {
    puts "ERROR: $::bmcaddr does not have firmware supporting Amester"
    exit_application
}
puts "$::theameclist"

puts "Server started on port $::port" 
set server [socket -server analyze_message $port]
puts "listening ... " 

proc WriteFiles_trace {filename sensor data_entries} {
	set filename1 $filename
	set filename2 $filename
	append filename1 "_header.csv"
	append filename2 "_data.csv"

	# write headers out
	puts -nonewline "Writing file: $filename1 ... "
	set file [open $filename1 "w"]
	for { set i 0}  {$i < [llength $sensor]} {incr i} {
		set sensor_name [lindex $sensor  $i]
		# puts "$i: $sensor_name"
		puts -nonewline $file "$sensor_name"
		if {$i < [llength $sensor]-1 } { 
			puts -nonewline $file ", "
		}
	}
	close $file
	puts "\[OK\]"

	# write data out 
	puts -nonewline "Writing file: $filename1 ... "
	set file [open $filename2 "w"]
	foreach e $data_entries {puts $file [join $e ", "]}
	close $file
	puts "\[OK\]"
}

proc analyze_message {sock addr port} {
    set stime [clock seconds]
    # puts "$stime : Server is analyzing incoming message ..."
    fconfigure $sock -buffering line
    fileevent $sock readable [list parse_message $sock]
}

proc parse_message {sock} {
    # puts "In parse message"
    if {[eof $sock] || [catch {gets $sock line}]} {
	# puts "in if body"
        close $sock
    } else {
		puts "GOT LINE: $line"
		set alive 1

        if {$line == "start"} {
            if {$::mode == "TRACE"} {
        		start_measuring_trace
        	} elseif {$::mode == "ACC"} {
				start_measuring_acc
        	} else {
        		puts "$::mode now konw."
        		exit_application
        	}
        } elseif {$line == "stop"} {
			if {$::mode == "TRACE"} {
        		stop_measuring_trace
        	} elseif {$::mode == "ACC"} {
				stop_measuring_acc
        	} else {
        		puts "$::mode not konw."
        		exit_application
        	}
        } elseif {$line == "close"} {
            puts "++++CLOSE++++"
            close $sock
            set alive 0
        } elseif { [regexp {config:(.*)} $line match configStr] == 1} {
        	config_measuring $configStr
		} elseif { [regexp {filebase:(.*)} $line match tmpStr] == 1} {
        	set ::OutFileBase $tmpStr
        	puts "Setting Output file base: $::OutFileBase"
        } else {
            puts "CMD NOT KNOWN: \" $line \""
			# exit_application
        }
        
        if {$alive==1} {
	    	puts "send ack"
        	puts $sock "ack"
    	}
    }
    puts "finished parse massege"
}


# PWR250US
# PWR250USFAN
# PWR250USGPU
# PWR250USIO
# PWR250USMEM0
# PWR250USP0
# PWR250USSTORE
# PWR250USVCS0
# PWR250USVDD0
# PWRAPSSCH0
# PWRAPSSCH1
# PWRAPSSCH10
# PWRAPSSCH11
# PWRAPSSCH12
# PWRAPSSCH13
# PWRAPSSCH14
# PWRAPSSCH15
# PWRAPSSCH2
# PWRAPSSCH3
# PWRAPSSCH4
# PWRAPSSCH5
# PWRAPSSCH6
# PWRAPSSCH7

# mysys_node0_ame0 get sensors
# mysys_node0_ame0
# mysys_node0_ame0_trace250us (FAST 4KHz )
# mysys_node0_ame0_trace2ms   (SLOW 500Hz)

# mysys_node0_ame0_PWR250US

 # Valid sensor fields are: 
 #       "value": sensor value (2 bytes)
 #       "min": minimum value (2 bytes)
 #       "max": maximum value (2 bytes)
 #       "acc": accumuated value (4 bytes)
 #       "updates": number of times sensor has been written (4 bytes)
 #       "test": test field (undefined) (2 bytes)
 #       "rcnt": The firmware 1 ms clock (4 bytes). Same value for all sensors.

proc config_acc {} {
	set ::allsensors {}

	foreach amec $::theameclist {
		# Reset Sensors min/max value.
		$amec clear_minmax_all_sync
	    $amec set_sensor_list $::my_sensor_list
	    # Make allsensors, a list of all sensor objects
	    set ::allsensors [concat $::allsensors [$amec get sensorname $::my_sensor_list]]
	}

	set ::mode "ACC"

	puts -nonewline "Waiting to initalize sensors ... "
	foreach s $::allsensors {
		$s wait value
	}
	puts "\[OK\]"

}

proc config_measuring {configStr} {
	puts "Configure Measurement setup"
	set ::mode "NOT DEFINED"
	#############################################################
	### ACCU BASED MEASUREMENTS
	#############################################################
	if {$configStr == "BASIC_PWR:ACC"} {
		set ::my_sensor_list {
			PWR250US
			PWR250USFAN
			PWR250USGPU
			PWR250USMEM0
			PWR250USP0
			PWR250USVCS0
			PWR250USVDD0
			PWR250USIO
			PWR250USSTORE
		}

		config_acc

    } elseif {$configStr == "DEV_001:ACC"} {
    	set ::my_sensor_list {
			FREQA2MSP0
			IPS2MSP0
			PWR250US
			TEMP2MSP0
			VOLT250USP0V0
		}

		config_acc

	} elseif {$configStr == "DEV_002:ACC"} {
    	set ::my_sensor_list {
			PWR250US
			TEMP2MSP0
		}

		config_acc
	#############################################################
	### TRACE BASED MEASUREMENTS
	#############################################################
    } elseif {$configStr == "DEV_001:TRACE"} {
		::mysys_node0_ame0_trace2ms set_config {{PWR250US rcnt} {PWR250US value} {PWR250USFAN value} {PWR250USGPU value} {PWR250USIO value}  {PWR250USMEM0 value}  {PWR250USP0 value}  {PWR250USSTORE value} {PWR250USVCS0 value} {PWR250USVDD0 value}  } {} {} {}
    	set ::mode "TRACE"

    } else {
        puts "configStr NOT KNOWN: $configStr"
    }

   	puts "Setting mode: $::mode"
}


#############################################################
### ACCU BASED MEASUREMENTS
#############################################################
proc myMean {acc1 acc2 up1 up2} {
   return [expr ($acc2-$acc1) / ($up2-$up1) ]
}

proc start_measuring_acc {} {
  	set ::acc_data {}

	set dataLine {}

	# Get the timestep for the first sensor only.
	# lappend dataLine "[[lindex $::allsensors  0] cget -timestamp]"

	foreach s $::allsensors {
		lappend dataLine "[$s cget -timestamp]"
		lappend dataLine "[$s cget -value_acc]"
		lappend dataLine "[$s cget -updates]"
	}

	lappend ::acc_data $dataLine
}

proc stop_measuring_acc {} {

	set dataLine {}

	# Get the timestep for the first sensor only.
	# lappend dataLine "[[lindex $::allsensors  0] cget -timestamp]"

	foreach s $::allsensors {
		lappend dataLine "[$s cget -value_acc]"
		lappend dataLine "[$s cget -value_acc]"
		lappend dataLine "[$s cget -updates]"
	}

	lappend ::acc_data $dataLine

	WriteFiles_trace $::OutFileBase $::allsensors $::acc_data
    # foreach e $var {puts stdout [join $e ","]}
}

#############################################################
### TRACE BASED MEASUREMENTS
### LIMITED IN TIME SPAN THAT CAN BE MEASURED. (8KB internal memory)
#############################################################
proc start_measuring_trace {} {
#	puts "Configure Measurement setup"
#	::mysys_node0_ame0_trace250us set_config {{PWR250US rcnt}} {} {} {}
#	::mysys_node0_ame0_trace250us set_config {{PWR250US rcnt} {PWR250US value} {PWR250US min} {PWR250US max}} {} {} {}
#   {XXX value}
#   ::mysys_node0_ame0_trace2ms set_config {{PWR250US rcnt} {PWR250US value} {PWR250USFAN value} {PWR250USGPU value} {PWR250USIO value}  {PWR250USMEM0 value}  {PWR250USP0 value}  {PWR250USSTORE value} {PWR250USVCS0 value} {PWR250USVDD0 value}  } {} {} {}
	puts "Start Trace"
	::mysys_node0_ame0_trace2ms start
}

proc stop_measuring_trace {} {
	::mysys_node0_ame0_trace2ms stop
	puts "Stop Trace"
	set entries [::mysys_node0_ame0_trace2ms get_all_entries]
	# foreach e $entries {puts stdout [join $e ","]}
	WriteFiles_trace "foo" "todo" $entries
}

vwait forever
