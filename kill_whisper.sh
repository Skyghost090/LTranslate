function main {
    killall whisper-server
    if [[ $? == 0 ]]; then
        main
    fi
}

main
