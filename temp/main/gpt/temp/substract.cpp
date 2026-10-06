if(found)
    if (len,id,inst 필드 읽기 가능?)
        if(len,id,inst 불가능헤더)
            // found = false
            // idx += header_len
        if (packet 필드 읽기 가능?)
            if(crc=calculated_crc)
                // found = false
                // idx, raed_av = 0
            

        else if(buff full)
            // idx까지 보낼 수 있는 만큼 보내고
            // 보낸만큼 당기기


    else // 데이터 부족
        // 추가로 읽어오기
    if (buff full)
        // idx까지 보낼 수 있는 만큼 보내고
        // 보낸만큼 당기기