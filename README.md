# Capstone Design 2023-1
# SIKE를 이용한 드론 Mavlink 강화
Hardening drone Mavlink with SIKE(영문)  

## Student List
* 고성훈(2017103951, kanaba5@khu.ac.kr)  

## Overview  
최근 여러 산업에서 드론이 사용되면서 보안 문제가 떠오르고 있다. 이 연구는 드론통신에서 보안을 강화하기 위해, 양자 암호학 기술인 Post-Quantum Cryptography(PQC)을 결합하여 드론 통신의 라이브러리 중 하나인 MAVLINK를 강화하는 방법을 제안한다.  
  이를 위해, PQC 알고리즘 중 하나인 SIKE 사용하여 효율적이고 안전한 공유키를 생성 후 libsodium을 통해 데이터를 암호화하고 복호화하여 안전한 통신을 구현하고 검증하고자 한다.  
구동환경은 debian11이다.

## Motivation
드론 산업은 농업, 방재, 산림, 해양 등 다양한 분야에서 사용되고 있으며, 무인 항공기의 사용 분야는 지속적으로 확대되고 있다. 이에 따라 드론 기술은 기존의 방식보다 더욱 높은 수준의 안전성과 보안성이 요구된다.   
  드론은 자동차와 달리 공중에서 움직이는 차량으로, 사용자가 조종하지 않을 경우 다양한 위험 요소가 존재한다. 그리고 특히 군사 드론인 경우 전송하는 데이터가 타인에게 노출될 경우 불법적인 스파이나 공격 등의 위협을 받을 수 있다. 따라서 드론 통신 보안 문제는 드론의 사용자 및 외부인에게 큰 문제로 작용할 수 있다.  
  드론 통신 보안 문제를 해결하기 위한 기존의 암호화 기술은 대체로 전통적인 RSA, AES 등의 기술을 사용하였다. 하지만 이러한 기술들은 양자 공격에는 취약다는 점이 있다. 그래서 미래에는 안전성이 보장되지 않는 기술로 인식되고 있다.
  이에 반해, 포스트-퀀텀 암호(PQC) 기술은 전통적인 암호화 기술의 한계를 극복하고, 향후 포스트-퀀텀 시대에 적용될 수 있는 안전한 암호화 기술로 주목받고 있다. 이러한 PQC 기술 중 Supersingular Isogeny Key Encapsulation(SIKE) 알고리즘은 안전성과 효율성 모두에서 우수한 성능을 보이고 있다.  
  SIKE를 이용하여 안전하고 보안성이 높은 공유키를 생성하고 이 공유키를 이용하여 주고받을 데이터를 암호화 및 복호화하여 통신함으로써 안전한 통신을 구현할 수 있다. 따라서 본 연구에서는 Mavlink 프로토콜에 SIKE 알고리즘을 추가하여 드론 통신 보안성을 높이는 방안을 제시하고자 한다.  
## Related research
  SIKE 알고리즘은 CANS’19에서 발표된 연구 결과에서 Montgomery multiplication을 최적화하고 연산자들의 파이프라이닝을 만족하는 형식으로 명령어셋을 조정함으로써 높은 연산 성능을 달성하는 것이 가능함을 보였다. 따라서 향후에 연구를 지속할 수 있다면 높은 연산 성능을 달성할 수 있는 방법을 고안한다면 더욱더 차별화된 PQC알고리즘으로 활용할 수 있을 것이다.  
  
연산 속도  
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/7.png)   
  
메모리 사용량  
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/8.png)   
(연산 단위: clock cycles)  
  
## Library
사용한 라이브러리 리스트  
1. MavLink - https://github.com/mavlink/mavlink
2. SIKE - https://github.com/microsoft/PQCrypto-SIDH
3. libsodium - https://github.com/jedisct1/libsodium  

각 라이브러리 공식 홈페이지에 들어가서 다운로드 후 빌드하여 이용  
  
## Project diagram
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/5.png)  
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/6.png)  
## Results  

암호화 복호화 없는 mavlink 통신  
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/1.png)   
  
SIKE로 비밀 공유키를 생성  
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/2.png)    
  
생성한 비밀 공유키를 이용하여 mavlink메세지를 암호화 한 후 드론으로 전송, 드론은 받은 mavlink 메세지를 복호화  
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/3.png)  
![image](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/4.png)   
사용자로부터 받은 오토파일럿 모드, 이륙, 목표 위치, 착륙 명령 암호화 후 전송.  
드론에서 수신 후 복호화하여 명령 처리.  


## Conclusion
  이 프로젝트를 통해 드론 통신 보안 강화를 위해 PQC 의 SIKE 알고리즘을 적용하고 시뮬레이션 및 테스트를 진행한다. 실험 결과, 기존 MAVLINK 통신에 비해 더욱안전하고 보안성이 높은 통신 방식을 제안한다. 특히 PQC를 이용한 SIKE 알고리즘의 적용으로, 기존의 RSA 나 DH 등 대칭키 알고리즘과 달리 공개키 기반의 암호화를 가능하게 하여 중간자 공격 등에 대한 취약성을 보완한다. 보안성이 더욱 강화된 드론 통신 시스템을 구축할 수 있다. 따라서 이 프로젝트를 통해 드론 통신 보안 분야에서의 기술 발전과 안정적인 통신시스템 구축에 기여할 수 있으며, 이를 통해 드론을 보다 안전하게 운용할 수 있는 효과가 기대된다.  
   SIKE는 아이소지니 기반 키교환 프로토콜을 KEM 으로 확장한 프로토콜로 ECC의 프리미티브 연산에 기반을 두고 있는 양자 내성 암호이다. SIKE는 매우 작은 키와 암호문의 크기를 가지기 때문에 앞으로도 드론과 같은 실시간 통신이 중요하고 하드웨어 제약이 있는 장치에 유용하게 쓰일 수 있다.
  
## References
[1] 박찬희. "mbedTLS 상에서 격자기반 양자내성암호의 구현 및 성능평가." 국내석사학위논문 부산대학교 대학원, 2020. 부산  
[2] 저성능 사물인터넷 상에서의 양자 내성 암호 구현. 정 보 보 호 학 회 지 제 30 권 제1호, 2020. 2  
[3] https://en.wikipedia.org/wiki/MAVLink  
[4] https://doc.libsodium.org  
[5] https://mavlink.io/kr  
[6] https://github.com/microsoft/PQCrypto-SIDH  
[7] https://sike.org  
[8] https://eprint.iacr.org/2016/413.pdf  

## Reports
[[고성훈]_최종보고서(docx)](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/img/4.png)  
[[고성훈]_최종보고서(pdf)](https://github.com/goragoraki/Apply_PQC_To_MavLink_Capstone_Design2/blob/main/%EC%B5%9C%EC%A2%85%EB%B3%B4%EA%B3%A0%EC%84%9C/%5B%EA%B3%A0%EC%84%B1%ED%9B%88%5D%EC%B5%9C%EC%A2%85%EB%B3%B4%EA%B3%A0%EC%84%9C.pdf)  
