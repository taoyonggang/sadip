#include "TcpRecv.h"

cn::ThreadSafeQueue<cn::seisys::rbx::comm::bean::multi::FusionPathDatas> TcpRecv::fusionQueue_;

unsigned int TcpRecv::N_CRC16(unsigned char* updata, unsigned int len)
{
	unsigned char uchCRCHi = 0xff;
	unsigned char uchCRCLo = 0xff;
	unsigned int uindex;
	while (len--)
	{
		uindex = uchCRCHi ^ *updata++;
		uchCRCHi = uchCRCLo ^ auchCRCHi[uindex];
		uchCRCLo = auchCRCLo[uindex];
	}
	return (uchCRCHi << 8 | uchCRCLo);
};
