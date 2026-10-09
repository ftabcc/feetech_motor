task(){
    while(true){
        if(rx_start_time!=0)
            if (elapsed_time > timeout)
                status.err |= timeout;
            else
                wait_time = timeout - elapsed_time;
        else
            wait_time = max;

        if(!(status.err & timeout))
            notify = wait(wait_time)
            if (!notify)
                // status.err |= timeout;

        while (true){
            if(!(status.err & timeout))
                result = process();

            if(result = success)
                // 완료된 패킷저장
                if(rxpacket_queue send != True)
                    // 이전 rxpacket 큐 reset
                    // 완성패킷 큐send 
                // idx, read_av 완료된 패킷만 비우고 당기기
                if(idx!=0)
                    // 0~idx전까지 debug용 바이트스트림 연결시키기.

            if(result = failure) // crc_err
                // 패킷이라 판단했던 바이트 보내기
                // idx += header_len;

            if(status.err & full)
                // 0 ~ idx까지 debug 바이트 최대한 보내기.
                // status.err &= ~full;
                
            if(status.err != 0)
                if(status.err & timeout)
                    // timeout정보 보내기
                    // idx,found,confirm,start,read_av초기화
                    // status.err &= ~timeout;
                if(status.err & cdc_err)
                    // usb연결상태 경고보내기.
                    // status.err &= ~cdc_err;
            
            if(result = pending)
                // cdc callback의 notify받도록 break
                break;
        }
    }
}