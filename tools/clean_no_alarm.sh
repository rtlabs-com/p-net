number_of_arguments=$#
directory=$1
mode_argument=$2

if [ "$number_of_arguments" -eq 0 ] || [ "$number_of_arguments" -gt 2 ]; then
    echo "Usage: clean_no_alarm.sh directory [-a]"
    echo
    echo "Remove wireshark files that don't contain Profinet alarm frames."
    echo
    echo "By default this script will examine the next-newest file in the directory."
    echo "Give the -a argument to scan all files in the directory."
    echo
    echo "Use the watch command to run this repeatedly:"
    echo "  watch -n 10 ./clean_no_alarm.sh DIRECTORYNAME"
    echo
    echo "where the -n argument specifies the number of seconds between each invocation."
    echo
    echo "This tool uses the tshark program, which must be installed."
    echo
    echo "To record the files it is recommended to use the tcpdump utility to continuously"
    echo "save data into 10 MByte files:"
    echo "  sudo tcpdump -i INTERFACENAME -C 10 -w recordingfile -K -n"
    echo "The directory that the files are saved into must have write permission for all:"
    echo "  chmod o+w DIRECTORYNAME"

    # Handling both sourced and executed script
    return 1 2>/dev/null
    exit 1
fi

# Delete a wireshark pcap file if it doesn't contain any Profinet alarm frames
examine_file () {
    full_path=$1
    echo "Examining the file: ${full_path}"

    file_description=`file ${full_path}`
    case "$file_description" in
    *capture*) ;;
    *        ) echo "  Not a wireshark file"; return ;;
    esac

    # Check if the file contains the Profinet frame ID for ALARM_PRIO_LOW
    number_of_alarm_frames=`tshark -r ${full_path} -T fields -e frame.number -Y "pn_rt.frame_id == 65025"  | wc -l`
    if [ "$number_of_alarm_frames" -eq 0 ]; then
        echo "  Deleting ${full_path} as it has no alarms"
        rm "$full_path"
    fi
}

if [ "$mode_argument" = "-a" ]
then
    # Examine all files in directory
    for file in $directory/*
    do
        examine_file "$file"
    done
else
    # Examine second newest file
    filename=`ls -1t $directory | sed -n '2p'`
    examine_file ${directory}/${filename}
fi
