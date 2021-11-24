
CC=$(CROSS)gcc
CPP=$(CROSS)g++
LD=$(CROSS)ld
AR=$(CROSS)ar


CFLAGS=-DWITH_AES_CBC=1

ifeq "$(CROSS)" "arm-linux-"
CFLAGS+=-DMOXA
endif


ifeq "$(CROSS)" "mipsel-linux-"
CFLAGS+=-DNEXUS
DEBUG=-Wall
IFLAGS=-I.
else
DEBUG=-Wall -static -O2
IFLAGS=-I. -I/usr/include
endif



ifeq "$(CROSS)" "mipsel-linux-"
LIBS=-lpthread -lrt -lcgic
LDFLAGS=${LIBS} 
else
LIBS=-lpthread -lrt -lmysqlclient -lcgic
LDFLAGS=${LIBS} -L/usr/lib/mysql -L/usr/lib
endif



ifeq "$(CROSS)" "arm-angstrom-linux-gnueabi-"
DEBUG=-Wall
IFLAGS=-I. -I/usr/local/angstrom/arm/include
LIBS=-lpthread -lrt -lcgic
LDFLAGS=${LIBS} -L/usr/local/angstrom/arm/lib
endif



ifeq "$(CROSS)" "arm-linux-"
DEBUG=-Wall
IFLAGS=-I. -I/usr/local/arm-linux/include
LIBS=-lpthread -lrt -lcgic
LDFLAGS=${LIBS} -L/usr/local/arm-linux/lib
endif



ifeq "$(CROSS)" "mac-linux-"
CFLAGS=-DMAC
CC=gcc
CPP=g++
LD=ld
AR=ar
DEBUG=-Wall
IFLAGS=-I/usr/include -I. 
LIBS=-lcgic
LDFLAGS=${LIBS} -L/usr/lib -L. 
endif







#########################################################################################
#########################################################################################

DEPS=

ifeq "$(CROSS)" "mipsel-linux-"
TARGET=relay fwd bctl gate mbeq tcpio ZdoNwkAddrReq WhoamiReq PwmSet PwmReq GpioReq PinHiLoSet ModbusReq ResetReq CompileDateReq MyIdSet ztree ZdoMgmtPermitJoinReq ModbusPreset ZdoMgmtRtgReq RtsTimeoutSet AdcReq UxBaudSet UxGcrSet ZdoMgmtLeaveReq ThermReq PhotoReq JboxReq PulseCountReq PulseCountSet PulseOutReq PxDirReq PxDirSet PinInOutSet PinDirSet PinSelSet PxSelReq ZdoMgmtLqiReq ZdoIeeeAddrReq AdcCfgReq AdcCfgSet PxSelSet SysResetReq ZdoEndDeviceAnnce ZdoUserDescSet TxCtrllReq TxCtrllSet P0INPReq P0INPSet P1INPReq P1INPSet P2INPReq P2INPSet L16_LIVPLAY L16_PAGE_START_END L16_HYBERNATE_ENABLE L16_DISPLAY SMA_GET_NET_START SMA_GET_NET SMA_CFG_NETADR SMA_GET_DATA NvReq NvSet L16_SERIAL_LIVEPLAY MODBUS_SERIAL_READ MODBUS_SERIAL_SET hex2float float2hex zconsole xorsum CRC16-RTU reverse-bytes LeaveReq gprmc PWM_SERIAL_REQ GprmcReq GprmcSet
TARGET+=WhoamiExtReq.cgi WhoamiExtRsp.cgi WhoamiReq.cgi WhoamiRsp.cgi 
TARGET+=PinHiLoExtSet.cgi PinHiLoExtRsp.cgi PinHiLoSet.cgi PinHiLoRsp.cgi 
TARGET+=GpioExtSet.cgi GpioExtReq.cgi GpioExtRsp.cgi GpioSet.cgi GpioReq.cgi GpioRsp.cgi 
TARGET+=PwmExtSet.cgi PwmExtReq.cgi PwmExtRsp.cgi PwmSet.cgi PwmReq.cgi PwmRsp.cgi 
TARGET+=ResetExtReq.cgi ResetExtRsp.cgi ResetReq.cgi ResetRsp.cgi 
TARGET+=CompileDateExtReq.cgi CompileDateExtRsp.cgi CompileDateReq.cgi CompileDateRsp.cgi 
TARGET+=PhotoExtReq.cgi PhotoExtRsp.cgi PhotoReq.cgi PhotoRsp.cgi 
TARGET+=ZdoIeeeAddrReq.cgi ZdoIeeeAddrRsp.cgi ZdoNwkAddrReq.cgi ZdoNwkAddrRsp.cgi 
TARGET+=ZdoMgmtPermitJoinReq.cgi ZdoMgmtPermitJoinRsp.cgi 
TARGET+=ZdoMgmtDirectJoinReq.cgi ZdoMgmtDirectJoinRsp.cgi 
TARGET+=ZdoMgmtLqiReq.cgi ZdoMgmtLqiRsp.cgi 
TARGET+=ZdoMgmtRtgReq.cgi ZdoMgmtRtgRsp.cgi 
TARGET+=ZdoMgmtLeaveReq.cgi ZdoMgmtLeaveRsp.cgi 
TARGET+=PxDirExtSet.cgi PxDirExtReq.cgi PxDirExtRsp.cgi PxDirSet.cgi PxDirReq.cgi PxDirRsp.cgi 
TARGET+=PxSelExtSet.cgi PxSelExtReq.cgi PxSelExtRsp.cgi PxSelSet.cgi PxSelReq.cgi PxSelRsp.cgi 
TARGET+=AdcExtReq.cgi AdcExtRsp.cgi AdcReq.cgi AdcRsp.cgi 
TARGET+=PulseOutExtReq.cgi PulseOutExtRsp.cgi PulseOutReq.cgi PulseOutRsp.cgi 
TARGET+=ModbusPresetExtReq.cgi ModbusPresetExtRsp.cgi ModbusPresetReq.cgi ModbusPresetRsp.cgi 
TARGET+=ModbusQryExtReq.cgi ModbusQryExtRsp.cgi ModbusQryReq.cgi ModbusQryRsp.cgi 
TARGET+=RtsTimeoutExtSet.cgi RtsTimeoutExtRsp.cgi RtsTimeoutSet.cgi RtsTimeoutRsp.cgi 
TARGET+=UxBaudExtSet.cgi UxBaudExtRsp.cgi UxBaudSet.cgi UxBaudRsp.cgi UxGcrExtSet.cgi UxGcrExtRsp.cgi UxGcrSet.cgi UxGcrRsp.cgi 
TARGET+=MyIdExtSet.cgi MyIdExtRsp.cgi MyIdSet.cgi MyIdRsp.cgi 
TARGET+=ochi
else
TARGET=relay fwd bctl gate mbeq tcpio ZdoNwkAddrReq WhoamiReq PwmSet PwmReq GpioReq PinHiLoSet ModbusReq ResetReq CompileDateReq MyIdSet ztree ZdoMgmtPermitJoinReq ModbusPreset ZdoMgmtRtgReq RtsTimeoutSet AdcReq UxBaudSet UxGcrSet ZdoMgmtLeaveReq ThermReq PhotoReq JboxReq PulseCountReq PulseCountSet PulseOutReq PxDirReq PxDirSet PinInOutSet PinDirSet PinSelSet PxSelReq ZdoMgmtLqiReq ZdoIeeeAddrReq AdcCfgReq AdcCfgSet PxSelSet SysResetReq ZdoEndDeviceAnnce ZdoUserDescSet TxCtrllReq TxCtrllSet P0INPReq P0INPSet P1INPReq P1INPSet P2INPReq P2INPSet L16_LIVPLAY L16_PAGE_START_END L16_HYBERNATE_ENABLE L16_DISPLAY SMA_GET_NET_START SMA_GET_NET SMA_CFG_NETADR SMA_GET_DATA NvReq NvSet L16_SERIAL_LIVEPLAY MODBUS_SERIAL_READ MODBUS_SERIAL_SET hex2float float2hex zconsole xorsum CRC16-RTU reverse-bytes LeaveReq gprmc PWM_SERIAL_REQ GprmcReq GprmcSet gconsole
TARGET+=WhoamiExtReq.cgi WhoamiExtRsp.cgi WhoamiReq.cgi WhoamiRsp.cgi 
TARGET+=PinHiLoExtSet.cgi PinHiLoExtRsp.cgi PinHiLoSet.cgi PinHiLoRsp.cgi 
TARGET+=GpioExtSet.cgi GpioExtReq.cgi GpioExtRsp.cgi GpioSet.cgi GpioReq.cgi GpioRsp.cgi 
TARGET+=PwmExtSet.cgi PwmExtReq.cgi PwmExtRsp.cgi PwmSet.cgi PwmReq.cgi PwmRsp.cgi 
TARGET+=ResetExtReq.cgi ResetExtRsp.cgi ResetReq.cgi ResetRsp.cgi 
TARGET+=CompileDateExtReq.cgi CompileDateExtRsp.cgi CompileDateReq.cgi CompileDateRsp.cgi 
TARGET+=PhotoExtReq.cgi PhotoExtRsp.cgi PhotoReq.cgi PhotoRsp.cgi 
TARGET+=ZdoIeeeAddrReq.cgi ZdoIeeeAddrRsp.cgi ZdoNwkAddrReq.cgi ZdoNwkAddrRsp.cgi 
TARGET+=ZdoMgmtPermitJoinReq.cgi ZdoMgmtPermitJoinRsp.cgi 
TARGET+=ZdoMgmtDirectJoinReq.cgi ZdoMgmtDirectJoinRsp.cgi 
TARGET+=ZdoMgmtLqiReq.cgi ZdoMgmtLqiRsp.cgi 
TARGET+=ZdoMgmtRtgReq.cgi ZdoMgmtRtgRsp.cgi 
TARGET+=ZdoMgmtLeaveReq.cgi ZdoMgmtLeaveRsp.cgi 
TARGET+=PxDirExtSet.cgi PxDirExtReq.cgi PxDirExtRsp.cgi PxDirSet.cgi PxDirReq.cgi PxDirRsp.cgi 
TARGET+=PxSelExtSet.cgi PxSelExtReq.cgi PxSelExtRsp.cgi PxSelSet.cgi PxSelReq.cgi PxSelRsp.cgi 
TARGET+=AdcExtReq.cgi AdcExtRsp.cgi AdcReq.cgi AdcRsp.cgi 
TARGET+=PulseOutExtReq.cgi PulseOutExtRsp.cgi PulseOutReq.cgi PulseOutRsp.cgi 
TARGET+=ModbusPresetExtReq.cgi ModbusPresetExtRsp.cgi ModbusPresetReq.cgi ModbusPresetRsp.cgi 
TARGET+=ModbusQryExtReq.cgi ModbusQryExtRsp.cgi ModbusQryReq.cgi ModbusQryRsp.cgi 
TARGET+=RtsTimeoutExtSet.cgi RtsTimeoutExtRsp.cgi RtsTimeoutSet.cgi RtsTimeoutRsp.cgi 
TARGET+=UxBaudExtSet.cgi UxBaudExtRsp.cgi UxBaudSet.cgi UxBaudRsp.cgi UxGcrExtSet.cgi UxGcrExtRsp.cgi UxGcrSet.cgi UxGcrRsp.cgi
TARGET+=MyIdExtSet.cgi MyIdExtRsp.cgi MyIdSet.cgi MyIdRsp.cgi 
endif



ifeq "$(CROSS)" "arm-angstrom-linux-gnueabi-"
TARGET=relay fwd bctl gate mbeq bost.cgi tcpio ZdoNwkAddrReq WhoamiReq PwmSet PwmReq GpioReq PinHiLoSet ModbusReq ResetReq CompileDateReq MyIdSet ztree ZdoMgmtPermitJoinReq ModbusPreset ZdoMgmtRtgReq RtsTimeoutSet AdcReq UxBaudSet UxGcrSet ZdoMgmtLeaveReq ThermReq PhotoReq JboxReq PulseCountReq PulseCountSet PulseOutReq PxDirReq PxDirSet PinInOutSet PinDirSet PinSelSet PxSelReq ZdoMgmtLqiReq ZdoIeeeAddrReq AdcCfgReq AdcCfgSet PxSelSet SysResetReq ZdoEndDeviceAnnce ZdoUserDescSet TxCtrllReq TxCtrllSet P0INPReq P0INPSet P1INPReq P1INPSet P2INPReq P2INPSet L16_LIVPLAY L16_PAGE_START_END L16_HYBERNATE_ENABLE L16_DISPLAY SMA_GET_NET_START SMA_GET_NET SMA_CFG_NETADR SMA_GET_DATA NvReq NvSet L16_SERIAL_LIVEPLAY MODBUS_SERIAL_READ MODBUS_SERIAL_SET hex2float float2hex zconsole xorsum CRC16-RTU reverse-bytes LeaveReq gprmc PWM_SERIAL_REQ GprmcReq GprmcSet
TARGET+=WhoamiExtReq.cgi WhoamiExtRsp.cgi WhoamiReq.cgi WhoamiRsp.cgi 
TARGET+=PinHiLoExtSet.cgi PinHiLoExtRsp.cgi PinHiLoSet.cgi PinHiLoRsp.cgi 
TARGET+=GpioExtSet.cgi GpioExtReq.cgi GpioExtRsp.cgi GpioSet.cgi GpioReq.cgi GpioRsp.cgi 
TARGET+=PwmExtSet.cgi PwmExtReq.cgi PwmExtRsp.cgi PwmSet.cgi PwmReq.cgi PwmRsp.cgi 
TARGET+=ResetExtReq.cgi ResetExtRsp.cgi ResetReq.cgi ResetRsp.cgi 
TARGET+=CompileDateExtReq.cgi CompileDateExtRsp.cgi CompileDateReq.cgi CompileDateRsp.cgi 
TARGET+=PhotoExtReq.cgi PhotoExtRsp.cgi PhotoReq.cgi PhotoRsp.cgi 
TARGET+=ZdoIeeeAddrReq.cgi ZdoIeeeAddrRsp.cgi ZdoNwkAddrReq.cgi ZdoNwkAddrRsp.cgi 
TARGET+=ZdoMgmtPermitJoinReq.cgi ZdoMgmtPermitJoinRsp.cgi 
TARGET+=ZdoMgmtDirectJoinReq.cgi ZdoMgmtDirectJoinRsp.cgi 
TARGET+=ZdoMgmtLqiReq.cgi ZdoMgmtLqiRsp.cgi 
TARGET+=ZdoMgmtRtgReq.cgi ZdoMgmtRtgRsp.cgi 
TARGET+=ZdoMgmtLeaveReq.cgi ZdoMgmtLeaveRsp.cgi 
TARGET+=PxDirExtSet.cgi PxDirExtReq.cgi PxDirExtRsp.cgi PxDirSet.cgi PxDirReq.cgi PxDirRsp.cgi 
TARGET+=PxSelExtSet.cgi PxSelExtReq.cgi PxSelExtRsp.cgi PxSelSet.cgi PxSelReq.cgi PxSelRsp.cgi 
TARGET+=AdcExtReq.cgi AdcExtRsp.cgi AdcReq.cgi AdcRsp.cgi 
TARGET+=PulseOutExtReq.cgi PulseOutExtRsp.cgi PulseOutReq.cgi PulseOutRsp.cgi 
TARGET+=ModbusPresetExtReq.cgi ModbusPresetExtRsp.cgi ModbusPresetReq.cgi ModbusPresetRsp.cgi 
TARGET+=ModbusQryExtReq.cgi ModbusQryExtRsp.cgi ModbusQryReq.cgi ModbusQryRsp.cgi 
TARGET+=RtsTimeoutExtSet.cgi RtsTimeoutExtRsp.cgi RtsTimeoutSet.cgi RtsTimeoutRsp.cgi 
TARGET+=UxBaudExtSet.cgi UxBaudExtRsp.cgi UxBaudSet.cgi UxBaudRsp.cgi UxGcrExtSet.cgi UxGcrExtRsp.cgi UxGcrSet.cgi UxGcrRsp.cgi 
TARGET+=MyIdExtSet.cgi MyIdExtRsp.cgi MyIdSet.cgi MyIdRsp.cgi psensor_getdata psensor_setdata
endif



ifeq "$(CROSS)" "arm-linux-"
TARGET=relay fwd bctl gate mbeq tcpio ZdoNwkAddrReq WhoamiReq PwmSet PwmReq GpioReq PinHiLoSet ModbusReq ResetReq CompileDateReq MyIdSet ztree ZdoMgmtPermitJoinReq ModbusPreset ZdoMgmtRtgReq RtsTimeoutSet AdcReq UxBaudSet UxGcrSet ZdoMgmtLeaveReq ThermReq PhotoReq JboxReq PulseCountReq PulseCountSet PulseOutReq PxDirReq PxDirSet PinInOutSet PinDirSet PinSelSet PxSelReq ZdoMgmtLqiReq ZdoIeeeAddrReq AdcCfgReq AdcCfgSet PxSelSet SysResetReq ZdoEndDeviceAnnce ZdoUserDescSet TxCtrllReq TxCtrllSet P0INPReq P0INPSet P1INPReq P1INPSet P2INPReq P2INPSet L16_LIVPLAY L16_PAGE_START_END L16_HYBERNATE_ENABLE L16_DISPLAY SMA_GET_NET_START SMA_GET_NET SMA_CFG_NETADR SMA_GET_DATA NvReq NvSet L16_SERIAL_LIVEPLAY MODBUS_SERIAL_READ MODBUS_SERIAL_SET hex2float float2hex zconsole xorsum CRC16-RTU reverse-bytes LeaveReq gprmc PWM_SERIAL_REQ GprmcReq GprmcSet
TARGET+=WhoamiExtReq.cgi WhoamiExtRsp.cgi WhoamiReq.cgi WhoamiRsp.cgi 
TARGET+=PinHiLoExtSet.cgi PinHiLoExtRsp.cgi PinHiLoSet.cgi PinHiLoRsp.cgi 
TARGET+=GpioExtSet.cgi GpioExtReq.cgi GpioExtRsp.cgi GpioSet.cgi GpioReq.cgi GpioRsp.cgi 
TARGET+=PwmExtSet.cgi PwmExtReq.cgi PwmExtRsp.cgi PwmSet.cgi PwmReq.cgi PwmRsp.cgi 
TARGET+=ResetExtReq.cgi ResetExtRsp.cgi ResetReq.cgi ResetRsp.cgi 
TARGET+=CompileDateExtReq.cgi CompileDateExtRsp.cgi CompileDateReq.cgi CompileDateRsp.cgi 
TARGET+=PhotoExtReq.cgi PhotoExtRsp.cgi PhotoReq.cgi PhotoRsp.cgi 
TARGET+=ZdoIeeeAddrReq.cgi ZdoIeeeAddrRsp.cgi ZdoNwkAddrReq.cgi ZdoNwkAddrRsp.cgi 
TARGET+=ZdoMgmtPermitJoinReq.cgi ZdoMgmtPermitJoinRsp.cgi 
TARGET+=ZdoMgmtDirectJoinReq.cgi ZdoMgmtDirectJoinRsp.cgi 
TARGET+=ZdoMgmtLqiReq.cgi ZdoMgmtLqiRsp.cgi 
TARGET+=ZdoMgmtRtgReq.cgi ZdoMgmtRtgRsp.cgi 
TARGET+=ZdoMgmtLeaveReq.cgi ZdoMgmtLeaveRsp.cgi 
TARGET+=PxDirExtSet.cgi PxDirExtReq.cgi PxDirExtRsp.cgi PxDirSet.cgi PxDirReq.cgi PxDirRsp.cgi 
TARGET+=PxSelExtSet.cgi PxSelExtReq.cgi PxSelExtRsp.cgi PxSelSet.cgi PxSelReq.cgi PxSelRsp.cgi 
TARGET+=AdcExtReq.cgi AdcExtRsp.cgi AdcReq.cgi AdcRsp.cgi 
TARGET+=PulseOutExtReq.cgi PulseOutExtRsp.cgi PulseOutReq.cgi PulseOutRsp.cgi 
TARGET+=ModbusPresetExtReq.cgi ModbusPresetExtRsp.cgi ModbusPresetReq.cgi ModbusPresetRsp.cgi 
TARGET+=ModbusQryExtReq.cgi ModbusQryExtRsp.cgi ModbusQryReq.cgi ModbusQryRsp.cgi 
TARGET+=RtsTimeoutExtSet.cgi RtsTimeoutExtRsp.cgi RtsTimeoutSet.cgi RtsTimeoutRsp.cgi 
TARGET+=UxBaudExtSet.cgi UxBaudExtRsp.cgi UxBaudSet.cgi UxBaudRsp.cgi UxGcrExtSet.cgi UxGcrExtRsp.cgi UxGcrSet.cgi UxGcrRsp.cgi 
TARGET+=MyIdExtSet.cgi MyIdExtRsp.cgi MyIdSet.cgi MyIdRsp.cgi 
endif


ifeq "$(CROSS)" "mac-linux-"
TARGET=relay fwd bctl gate mbeq tcpio ZdoNwkAddrReq WhoamiReq PwmSet PwmReq GpioReq PinHiLoSet ModbusReq ResetReq CompileDateReq MyIdSet ztree ZdoMgmtPermitJoinReq ModbusPreset ZdoMgmtRtgReq RtsTimeoutSet AdcReq UxBaudSet UxGcrSet ZdoMgmtLeaveReq ThermReq PhotoReq JboxReq PulseCountReq PulseCountSet PulseOutReq PxDirReq PxDirSet PinInOutSet PinDirSet PinSelSet PxSelReq ZdoMgmtLqiReq ZdoIeeeAddrReq AdcCfgReq AdcCfgSet PxSelSet SysResetReq ZdoEndDeviceAnnce ZdoUserDescSet TxCtrllReq TxCtrllSet P0INPReq P0INPSet P1INPReq P1INPSet P2INPReq P2INPSet L16_LIVPLAY L16_PAGE_START_END L16_HYBERNATE_ENABLE L16_DISPLAY SMA_GET_NET_START SMA_GET_NET SMA_CFG_NETADR SMA_GET_DATA NvReq NvSet L16_SERIAL_LIVEPLAY MODBUS_SERIAL_READ MODBUS_SERIAL_SET hex2float float2hex zconsole xorsum CRC16-RTU reverse-bytes LeaveReq gprmc PWM_SERIAL_REQ GprmcReq GprmcSet
TARGET+=WhoamiExtReq.cgi WhoamiExtRsp.cgi WhoamiReq.cgi WhoamiRsp.cgi 
TARGET+=PinHiLoExtSet.cgi PinHiLoExtRsp.cgi PinHiLoSet.cgi PinHiLoRsp.cgi 
TARGET+=GpioExtSet.cgi GpioExtReq.cgi GpioExtRsp.cgi GpioSet.cgi GpioReq.cgi GpioRsp.cgi 
TARGET+=PwmExtSet.cgi PwmExtReq.cgi PwmExtRsp.cgi PwmSet.cgi PwmReq.cgi PwmRsp.cgi 
TARGET+=ResetExtReq.cgi ResetExtRsp.cgi ResetReq.cgi ResetRsp.cgi 
TARGET+=CompileDateExtReq.cgi CompileDateExtRsp.cgi CompileDateReq.cgi CompileDateRsp.cgi 
TARGET+=PhotoExtReq.cgi PhotoExtRsp.cgi PhotoReq.cgi PhotoRsp.cgi 
TARGET+=ZdoIeeeAddrReq.cgi ZdoIeeeAddrRsp.cgi ZdoNwkAddrReq.cgi ZdoNwkAddrRsp.cgi 
TARGET+=ZdoMgmtPermitJoinReq.cgi ZdoMgmtPermitJoinRsp.cgi 
TARGET+=ZdoMgmtDirectJoinReq.cgi ZdoMgmtDirectJoinRsp.cgi 
TARGET+=ZdoMgmtLqiReq.cgi ZdoMgmtLqiRsp.cgi 
TARGET+=ZdoMgmtRtgReq.cgi ZdoMgmtRtgRsp.cgi 
TARGET+=ZdoMgmtLeaveReq.cgi ZdoMgmtLeaveRsp.cgi 
TARGET+=PxDirExtSet.cgi PxDirExtReq.cgi PxDirExtRsp.cgi PxDirSet.cgi PxDirReq.cgi PxDirRsp.cgi 
TARGET+=PxSelExtSet.cgi PxSelExtReq.cgi PxSelExtRsp.cgi PxSelSet.cgi PxSelReq.cgi PxSelRsp.cgi 
TARGET+=AdcExtReq.cgi AdcExtRsp.cgi AdcReq.cgi AdcRsp.cgi 
TARGET+=PulseOutExtReq.cgi PulseOutExtRsp.cgi PulseOutReq.cgi PulseOutRsp.cgi 
TARGET+=ModbusPresetExtReq.cgi ModbusPresetExtRsp.cgi ModbusPresetReq.cgi ModbusPresetRsp.cgi 
TARGET+=ModbusQryExtReq.cgi ModbusQryExtRsp.cgi ModbusQryReq.cgi ModbusQryRsp.cgi 
TARGET+=RtsTimeoutExtSet.cgi RtsTimeoutExtRsp.cgi RtsTimeoutSet.cgi RtsTimeoutRsp.cgi 
TARGET+=UxBaudExtSet.cgi UxBaudExtRsp.cgi UxBaudSet.cgi UxBaudRsp.cgi UxGcrExtSet.cgi UxGcrExtRsp.cgi UxGcrSet.cgi UxGcrRsp.cgi 
TARGET+=MyIdExtSet.cgi MyIdExtRsp.cgi MyIdSet.cgi MyIdRsp.cgi 
endif






#########################################################################################
#########################################################################################
all: $(TARGET)

cls:
	@/usr/bin/clear

install: relay gate fwd bctl ztree WhoamiReq CompileDateReq MyIdSet PinHiLoSet GpioReq PwmSet PwmReq JboxReq PhotoReq ThermReq UxGcrSet UxBaudSet RtsTimeoutSet AdcReq ModbusPreset ModbusReq ResetReq ZdoNwkAddrReq ZdoMgmtRtgReq ZdoMgmtLeaveReq ZdoMgmtPermitJoinReq PulseCountReq PulseCountSet PulseOutReq PxDirReq PxDirSet PinInOutSet PinDirSet PinSelSet PxSelReq ZdoMgmtLqiReq ZdoIeeeAddrReq ZdoIeeeAddrReq AdcCfgReq AdcCfgSet PxSelSet SysResetReq ZdoEndDeviceAnnce ZdoUserDescSet TxCtrllReq TxCtrllSet P0INPReq P0INPSet P1INPReq P1INPSet P2INPReq P2INPSet L16_LIVPLAY L16_PAGE_START_END L16_HYBERNATE_ENABLE L16_DISPLAY SMA_GET_NET_START SMA_GET_NET SMA_CFG_NETADR SMA_GET_DATA NvReq NvSet L16_SERIAL_LIVEPLAY MODBUS_SERIAL_READ MODBUS_SERIAL_SET hex2float float2hex zconsole xorsum CRC16-RTU reverse-bytes LeaveReq gprmc PWM_SERIAL_REQ GprmcReq GprmcSet
	@/bin/echo ""
	@/bin/echo "MOVING REQUIRED ROUTINES TO \"/usr/local/bin/\" "
	@/bin/echo "..."
	@/bin/mv relay /usr/local/bin/
	@/bin/mv gate /usr/local/bin/
	@/bin/mv fwd /usr/local/bin/
	@/bin/mv bctl /usr/local/bin/
	@/bin/mv ztree /usr/local/bin/
	@/bin/mv WhoamiReq /usr/local/bin/
	@/bin/mv CompileDateReq /usr/local/bin/
	@/bin/mv MyIdSet /usr/local/bin/
	@/bin/mv PinHiLoSet /usr/local/bin/
	@/bin/mv PwmReq /usr/local/bin/
	@/bin/mv PwmSet /usr/local/bin/
	@/bin/mv GpioReq /usr/local/bin/
	@/bin/mv JboxReq /usr/local/bin/
	@/bin/mv PhotoReq /usr/local/bin/
	@/bin/mv ThermReq /usr/local/bin/
	@/bin/mv UxGcrSet /usr/local/bin/
	@/bin/mv UxBaudSet /usr/local/bin/
	@/bin/mv RtsTimeoutSet /usr/local/bin/
	@/bin/mv AdcReq /usr/local/bin/
	@/bin/mv ModbusPreset /usr/local/bin/
	@/bin/mv ModbusReq /usr/local/bin/
	@/bin/mv ResetReq /usr/local/bin/
	@/bin/mv ZdoNwkAddrReq /usr/local/bin/
	@/bin/mv ZdoMgmtRtgReq /usr/local/bin/
	@/bin/mv ZdoMgmtLqiReq /usr/local/bin/
	@/bin/mv ZdoMgmtLeaveReq /usr/local/bin/
	@/bin/mv ZdoMgmtPermitJoinReq /usr/local/bin/
	@/bin/mv PulseOutReq /usr/local/bin/
	@/bin/mv PulseCountSet /usr/local/bin/
	@/bin/mv PulseCountReq /usr/local/bin/
	@/bin/mv PxDirReq /usr/local/bin/
	@/bin/mv PxDirSet /usr/local/bin/
	@/bin/mv PinInOutSet /usr/local/bin/
	@/bin/mv PinDirSet /usr/local/bin/
	@/bin/mv PinSelSet /usr/local/bin/
	@/bin/mv PxSelReq /usr/local/bin/
	@/bin/mv ZdoIeeeAddrReq /usr/local/bin/
	@/bin/mv AdcCfgReq /usr/local/bin/
	@/bin/mv AdcCfgSet /usr/local/bin/
	@/bin/mv PxSelSet /usr/local/bin/
	@/bin/mv SysResetReq /usr/local/bin/
	@/bin/mv ZdoEndDeviceAnnce /usr/local/bin/
	@/bin/mv ZdoUserDescSet /usr/local/bin/
	@/bin/mv TxCtrllReq /usr/local/bin/
	@/bin/mv TxCtrllSet /usr/local/bin/
	@/bin/mv P0INPReq /usr/local/bin/
	@/bin/mv P0INPSet /usr/local/bin/
	@/bin/mv P1INPReq /usr/local/bin/
	@/bin/mv P1INPSet /usr/local/bin/
	@/bin/mv P2INPReq /usr/local/bin/
	@/bin/mv P2INPSet /usr/local/bin/
	@/bin/mv L16_LIVPLAY /usr/local/bin/
	@/bin/mv L16_PAGE_START_END /usr/local/bin/
	@/bin/mv L16_HYBERNATE_ENABLE /usr/local/bin/
	@/bin/mv L16_DISPLAY /usr/local/bin/
	@/bin/mv SMA_GET_NET_START /usr/local/bin/
	@/bin/mv SMA_GET_NET /usr/local/bin/
	@/bin/mv SMA_CFG_NETADR /usr/local/bin/
	@/bin/mv SMA_GET_DATA /usr/local/bin/
	@/bin/mv NvReq /usr/local/bin/
	@/bin/mv NvSet /usr/local/bin/
	@/bin/mv L16_SERIAL_LIVEPLAY /usr/local/bin/
	@/bin/mv MODBUS_SERIAL_READ /usr/local/bin/
	@/bin/mv MODBUS_SERIAL_SET /usr/local/bin/
	@/bin/mv hex2float /usr/local/bin/
	@/bin/mv float2hex /usr/local/bin/
	@/bin/mv xorsum /usr/local/bin/
	@/bin/mv CRC16-RTU /usr/local/bin/
	@/bin/mv reverse-bytes /usr/local/bin/
	@/bin/mv LeaveReq /usr/local/bin/
	@/bin/mv gprmc /usr/local/bin/
	@/bin/mv PWM_SERIAL_REQ /usr/local/bin/
	@/bin/mv GprmcReq /usr/local/bin/
	@/bin/mv GprmcSet /usr/local/bin/
	@/bin/sleep 1
	@/bin/echo "DONE "
	@/bin/echo ""



#########################################################################################
#########################################################################################

psensor_setdata: psensor_setdata.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

psensor_getdata: psensor_getdata.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

GprmcSet: GprmcSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

GprmcReq: GprmcReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PWM_SERIAL_REQ: PWM_SERIAL_REQ.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

gprmc: gprmc.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

LeaveReq: LeaveReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

reverse-bytes: reverse-bytes.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

CRC16-RTU: CRC16-RTU.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

xorsum: xorsum.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

gconsole: gconsole.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

zconsole: zconsole.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

float2hex: float2hex.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

hex2float: hex2float.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

MODBUS_SERIAL_SET: MODBUS_SERIAL_SET.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

MODBUS_SERIAL_READ: MODBUS_SERIAL_READ.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

NvSet: NvSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

NvReq: NvReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

SMA_GET_DATA: SMA_GET_DATA.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

SMA_CFG_NETADR: SMA_CFG_NETADR.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

SMA_GET_NET: SMA_GET_NET.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

SMA_GET_NET_START: SMA_GET_NET_START.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

L16_SERIAL_LIVEPLAY: L16_SERIAL_LIVEPLAY.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

L16_DISPLAY: L16_DISPLAY.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

L16_HYBERNATE_ENABLE: L16_HYBERNATE_ENABLE.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

L16_PAGE_START_END: L16_PAGE_START_END.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

L16_LIVPLAY: L16_LIVPLAY.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

P2INPSet: P2INPSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

P2INPReq: P2INPReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

P1INPSet: P1INPSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

P1INPReq: P1INPReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

P0INPSet: P0INPSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

P0INPReq: P0INPReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

TxCtrllSet: TxCtrllSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

TxCtrllReq: TxCtrllReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoUserDescSet: ZdoUserDescSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoEndDeviceAnnce: ZdoEndDeviceAnnce.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

SysResetReq: SysResetReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PxSelSet: PxSelSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

AdcCfgSet: AdcCfgSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

AdcCfgReq: AdcCfgReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoIeeeAddrReq: ZdoIeeeAddrReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PxSelReq: PxSelReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PinSelSet: PinSelSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PinDirSet: PinDirSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PinInOutSet: PinInOutSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PxDirSet: PxDirSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PxDirReq: PxDirReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PulseOutReq: PulseOutReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PulseCountSet: PulseCountSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PulseCountReq: PulseCountReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

JboxReq: JboxReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PhotoReq: PhotoReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ThermReq: ThermReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoMgmtLqiReq: ZdoMgmtLqiReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoMgmtLeaveReq: ZdoMgmtLeaveReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

UxGcrSet: UxGcrSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

UxBaudSet: UxBaudSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

AdcReq: AdcReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

RtsTimeoutSet: RtsTimeoutSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoMgmtRtgReq: ZdoMgmtRtgReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ModbusPreset: ModbusPreset.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoMgmtPermitJoinReq: ZdoMgmtPermitJoinReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ztree: ztree.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

MyIdSet: MyIdSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

CompileDateReq: CompileDateReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ResetReq: ResetReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ModbusReq: ModbusReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PinHiLoSet: PinHiLoSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

GpioReq: GpioReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PwmReq: PwmReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

PwmSet: PwmSet.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

WhoamiReq: WhoamiReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ZdoNwkAddrReq: ZdoNwkAddrReq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)


#########################################################################################
#########################################################################################

GateSrv: GateSrv.o cmn.o misc.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o cmn.o misc.o $(LDFLAGS)

relay: relay.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

gate: gate.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

fwd: fwd.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

bctl: bctl.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

tcpio: tcpio.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

mbeq: mbeq.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

disp: disp.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

ochi: ochi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $@.o $(LDFLAGS)

bost.cgi: bost.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) bost.o $(LDFLAGS)


#########################################################################################
#########################################################################################

ZdoMgmtDirectJoinReq.cgi: ZdoMgmtDirectJoinReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtDirectJoinReqCgi.o $(LDFLAGS)

ZdoMgmtDirectJoinRsp.cgi: ZdoMgmtDirectJoinRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtDirectJoinRspCgi.o $(LDFLAGS)

ZdoMgmtLeaveReq.cgi: ZdoMgmtLeaveReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtLeaveReqCgi.o $(LDFLAGS)

ZdoMgmtLeaveRsp.cgi: ZdoMgmtLeaveRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtLeaveRspCgi.o $(LDFLAGS)

ZdoMgmtRtgReq.cgi: ZdoMgmtRtgReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtRtgReqCgi.o $(LDFLAGS)

ZdoMgmtRtgRsp.cgi: ZdoMgmtRtgRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtRtgRspCgi.o $(LDFLAGS)

ZdoMgmtLqiReq.cgi: ZdoMgmtLqiReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtLqiReqCgi.o $(LDFLAGS)

ZdoMgmtLqiRsp.cgi: ZdoMgmtLqiRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtLqiRspCgi.o $(LDFLAGS)

ZdoMgmtPermitJoinReq.cgi: ZdoMgmtPermitJoinReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtPermitJoinReqCgi.o $(LDFLAGS)

ZdoMgmtPermitJoinRsp.cgi: ZdoMgmtPermitJoinRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoMgmtPermitJoinRspCgi.o $(LDFLAGS)

ZdoNwkAddrReq.cgi: ZdoNwkAddrReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoNwkAddrReqCgi.o $(LDFLAGS)

ZdoNwkAddrRsp.cgi: ZdoNwkAddrRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoNwkAddrRspCgi.o $(LDFLAGS)

ZdoIeeeAddrReq.cgi: ZdoIeeeAddrReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoIeeeAddrReqCgi.o $(LDFLAGS)

ZdoIeeeAddrRsp.cgi: ZdoIeeeAddrRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ZdoIeeeAddrRspCgi.o $(LDFLAGS)

PhotoExtReq.cgi: PhotoExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PhotoExtReqCgi.o $(LDFLAGS)

PhotoExtRsp.cgi: PhotoExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PhotoExtRspCgi.o $(LDFLAGS)

PhotoReq.cgi: PhotoReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PhotoReqCgi.o $(LDFLAGS)

PhotoRsp.cgi: PhotoRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PhotoRspCgi.o $(LDFLAGS)

CompileDateExtReq.cgi: CompileDateExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) CompileDateExtReqCgi.o $(LDFLAGS)

CompileDateExtRsp.cgi: CompileDateExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) CompileDateExtRspCgi.o $(LDFLAGS)

CompileDateReq.cgi: CompileDateReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) CompileDateReqCgi.o $(LDFLAGS)

CompileDateRsp.cgi: CompileDateRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) CompileDateRspCgi.o $(LDFLAGS)

ResetExtReq.cgi: ResetExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ResetExtReqCgi.o $(LDFLAGS)

ResetExtRsp.cgi: ResetExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ResetExtRspCgi.o $(LDFLAGS)

ResetReq.cgi: ResetReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ResetReqCgi.o $(LDFLAGS)

ResetRsp.cgi: ResetRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ResetRspCgi.o $(LDFLAGS)

PwmExtSet.cgi: PwmExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PwmExtSetCgi.o $(LDFLAGS)

PwmExtReq.cgi: PwmExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PwmExtReqCgi.o $(LDFLAGS)

PwmExtRsp.cgi: PwmExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PwmExtRspCgi.o $(LDFLAGS)

PwmSet.cgi: PwmSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PwmSetCgi.o $(LDFLAGS)

PwmReq.cgi: PwmReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PwmReqCgi.o $(LDFLAGS)

PwmRsp.cgi: PwmRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PwmRspCgi.o $(LDFLAGS)

GpioExtSet.cgi: GpioExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) GpioExtSetCgi.o $(LDFLAGS)

GpioExtReq.cgi: GpioExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) GpioExtReqCgi.o $(LDFLAGS)

GpioExtRsp.cgi: GpioExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) GpioExtRspCgi.o $(LDFLAGS)

GpioSet.cgi: GpioSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) GpioSetCgi.o $(LDFLAGS)

GpioReq.cgi: GpioReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) GpioReqCgi.o $(LDFLAGS)

GpioRsp.cgi: GpioRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) GpioRspCgi.o $(LDFLAGS)

PinHiLoExtSet.cgi: PinHiLoExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PinHiLoExtSetCgi.o $(LDFLAGS)

PinHiLoExtRsp.cgi: PinHiLoExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PinHiLoExtRspCgi.o $(LDFLAGS)

PinHiLoSet.cgi: PinHiLoSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PinHiLoSetCgi.o $(LDFLAGS)

PinHiLoRsp.cgi: PinHiLoRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PinHiLoRspCgi.o $(LDFLAGS)

WhoamiExtReq.cgi: WhoamiExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) WhoamiExtReqCgi.o $(LDFLAGS)

WhoamiReq.cgi: WhoamiReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) WhoamiReqCgi.o $(LDFLAGS)

WhoamiExtRsp.cgi: WhoamiExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) WhoamiExtRspCgi.o $(LDFLAGS)

WhoamiRsp.cgi: WhoamiRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) WhoamiRspCgi.o $(LDFLAGS)

PxDirExtSet.cgi: PxDirExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxDirExtSetCgi.o $(LDFLAGS)

PxDirExtReq.cgi: PxDirExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxDirExtReqCgi.o $(LDFLAGS)

PxDirExtRsp.cgi: PxDirExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxDirExtRspCgi.o $(LDFLAGS)

PxDirSet.cgi: PxDirSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxDirSetCgi.o $(LDFLAGS)

PxDirReq.cgi: PxDirReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxDirReqCgi.o $(LDFLAGS)

PxDirRsp.cgi: PxDirRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxDirRspCgi.o $(LDFLAGS)

AdcExtReq.cgi: AdcExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) AdcExtReqCgi.o $(LDFLAGS)

AdcExtRsp.cgi: AdcExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) AdcExtRspCgi.o $(LDFLAGS)

AdcReq.cgi: AdcReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) AdcReqCgi.o $(LDFLAGS)

AdcRsp.cgi: AdcRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) AdcRspCgi.o $(LDFLAGS)

PxSelExtSet.cgi: PxSelExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxSelExtSetCgi.o $(LDFLAGS)

PxSelExtReq.cgi: PxSelExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxSelExtReqCgi.o $(LDFLAGS)

PxSelExtRsp.cgi: PxSelExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxSelExtRspCgi.o $(LDFLAGS)

PxSelSet.cgi: PxSelSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxSelSetCgi.o $(LDFLAGS)

PxSelReq.cgi: PxSelReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxSelReqCgi.o $(LDFLAGS)

PxSelRsp.cgi: PxSelRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PxSelRspCgi.o $(LDFLAGS)

PulseOutExtReq.cgi: PulseOutExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PulseOutExtReqCgi.o $(LDFLAGS)

PulseOutExtRsp.cgi: PulseOutExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PulseOutExtRspCgi.o $(LDFLAGS)

PulseOutReq.cgi: PulseOutReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PulseOutReqCgi.o $(LDFLAGS)

PulseOutRsp.cgi: PulseOutRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) PulseOutRspCgi.o $(LDFLAGS)

ModbusQryExtReq.cgi: ModbusQryExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusQryExtReqCgi.o $(LDFLAGS)

ModbusQryExtRsp.cgi: ModbusQryExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusQryExtRspCgi.o $(LDFLAGS)

ModbusQryReq.cgi: ModbusQryReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusQryReqCgi.o $(LDFLAGS)

ModbusQryRsp.cgi: ModbusQryRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusQryRspCgi.o $(LDFLAGS)

RtsTimeoutExtSet.cgi: RtsTimeoutExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) RtsTimeoutExtSetCgi.o $(LDFLAGS)

RtsTimeoutExtRsp.cgi: RtsTimeoutExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) RtsTimeoutExtRspCgi.o $(LDFLAGS)

RtsTimeoutSet.cgi: RtsTimeoutSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) RtsTimeoutSetCgi.o $(LDFLAGS)

RtsTimeoutRsp.cgi: RtsTimeoutRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) RtsTimeoutRspCgi.o $(LDFLAGS)

UxBaudExtSet.cgi: UxBaudExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxBaudExtSetCgi.o $(LDFLAGS)

UxBaudExtRsp.cgi: UxBaudExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxBaudExtRspCgi.o $(LDFLAGS)

UxBaudSet.cgi: UxBaudSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxBaudSetCgi.o $(LDFLAGS)

UxBaudRsp.cgi: UxBaudRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxBaudRspCgi.o $(LDFLAGS)

UxGcrExtSet.cgi: UxGcrExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxGcrExtSetCgi.o $(LDFLAGS)

UxGcrExtRsp.cgi: UxGcrExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxGcrExtRspCgi.o $(LDFLAGS)

UxGcrSet.cgi: UxGcrSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxGcrSetCgi.o $(LDFLAGS)

UxGcrRsp.cgi: UxGcrRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) UxGcrRspCgi.o $(LDFLAGS)

MyIdExtSet.cgi: MyIdExtSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) MyIdExtSetCgi.o $(LDFLAGS)

MyIdExtRsp.cgi: MyIdExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) MyIdExtRspCgi.o $(LDFLAGS)

MyIdSet.cgi: MyIdSetCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) MyIdSetCgi.o $(LDFLAGS)

MyIdRsp.cgi: MyIdRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) MyIdRspCgi.o $(LDFLAGS)

ModbusPresetExtReq.cgi: ModbusPresetExtReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusPresetExtReqCgi.o $(LDFLAGS)

ModbusPresetExtRsp.cgi: ModbusPresetExtRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusPresetExtRspCgi.o $(LDFLAGS)

ModbusPresetReq.cgi: ModbusPresetReqCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusPresetReqCgi.o $(LDFLAGS)

ModbusPresetRsp.cgi: ModbusPresetRspCgi.o
	$(CPP) -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) ModbusPresetRspCgi.o $(LDFLAGS)


#########################################################################################
#########################################################################################

.c.o:
	$(CC) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<

.cpp.o:
	$(CPP) -c -o $@ $(DEBUG) $(CFLAGS) $(IFLAGS) $<

clean:
	rm -f *.cgi tcpio mbeq disp topology *.cgi *.o *.gdb *.elf* core a.out out.txt out.bin nohup* .#* $(TARGET) *.txt ochi *Test *test zconsole gconsole

