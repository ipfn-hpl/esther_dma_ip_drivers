/**
 * @file atcadaq.c
 * @brief Example application for the ATCA-MIMO-V2 boards
 * @author Bernardo Carvalho
 * @date 01/06/2021
 *
 * @copyright Copyright 2016 - 2021 IPFN-Instituto Superior Tecnico, Portugal
 * Licensed under the EUPL, Version 1.2 only (the "Licence");
 * You may not use this work except in compliance with the Licence.
 * You may obtain a copy of the Licence, available in 23 official languages of
 * the European Union, at:
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl-text-11-12
 *
 * @warning Unless required by applicable law or agreed to in writing, software
 * distributed under the Licence is distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the Licence for the specific language governing permissions and
 * limitations under the Licence.
 *
 * @depends ATCA V2 PCI ADC xdma  device driver
 * @details
 */

/*---------------------------------------------------------------------------*/
/*                        Standard header includes                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>atca
#include <signal.h>
//#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

/*---------------------------------------------------------------------------*/
/*                        Project header includes                            */
/*---------------------------------------------------------------------------*/
#include "esther-daq.h"
#include "esther-daq-lib.h"

//#define N_PACKETS 2  // size in full buffers (2MB) of data to save to disk. 2M=64 kSample =16 ms
//Global vars
//int run=1;


void getkey ()
{
    printf("Press Return to continue ...\n");
    getchar();
}
void save_to_bin(int dmasize, short *acqData){
    FILE *stream_out;
    char path_name[80];
    //stream_out = fopen("data/out_fmc.bin","wb");
    sprintf(path_name,"data/out_fmc.bin");
    stream_out = fopen(path_name,"wb");
    fwrite(acqData, 1, dmasize, stream_out);
//    fflush(stream_out);
    fclose(stream_out);
}
void save_to_disk(int dmasize, short *acqData){
    FILE *stream_out;
    char file_name[40];
    char dir_name[40];
    char path_name[80];

    int numchan;
    int nsample;
    time_t now;
    struct tm *t;

    // Build directory name based on current date and time
    now = time(NULL);
    t = localtime(&now);
    strftime(dir_name, sizeof(dir_name)-1, "%Y%m%d%H%M%S", t);

    printf("Saving data to disk ...\n");
    printf("\ndir_name: %s\n", dir_name);

    mkdir(dir_name,0777); // Create directory

    for (numchan = 0; numchan < 4; numchan++) {
        // Create complete pathname for data file
        sprintf(file_name,"/ch%2.2d.txt", numchan+1);
        //printf("file_name: %s\n", file_name);
        strcpy(path_name, dir_name);
        strcat(path_name, file_name);
        //printf("path_name: %s\n", path_name);
        stream_out = fopen(path_name,"wt");
        // Write data of each channel  into a separate file (max 32)
        for (nsample=0; nsample < dmasize/4/2;  nsample++) {

            fprintf(stream_out, "%.4hd ",acqData[(nsample*4 )+ numchan]);
            fprintf(stream_out, "\n");
        }
		fflush(stream_out);
		fclose(stream_out);
//        printf("%d values for channel %d saved\n", nrOfReads*N_AQS, numchan+1);
    }


    printf("Saving data to disk finished\n");

}


int main(int argc, char *argv[])
{
    int dev_num = 0;
    int i,  k;  					// loop iterators i
    int rc;
    //int fd;
    char *arg = "-sw";
    int hw_trigger = 0; 				// trigger scheme to start acquisition
    void *map_base;//, *virt_addr;
    int fd_bar_0, fd_bar_1;   // file descriptor of device2
    short * acqData; //[ N_AQS * N_PACKETS * 2 *NUM_CHAN];	// large buffer to store data for 2096 ms which can be saved to hard drive
    uint32_t control_reg;
    shapi_device_info * pDevice;
    shapi_module_info * pModule;
    uint32_t trigOff32, dly; 

    int dmasize= 1048576*3; // 0x100000   maxx ~ 132k Samples

    // Check command line
    if(argc < 2) {
        printf("Usage: estherdaq  -hw|-sw [size]\n");
        exit(-1);
    }
    arg = argv[1];
    if (!strcmp(arg, "-hw")){
        hw_trigger = 1;
    }
    printf("arg1 = %s\n", arg);

    if(argc > 2){
        arg = argv[2];
        printf("arg2 = %s\n", arg);
        //dev_name = argv[3];
        dmasize = atoi(arg);
        printf("arg2n = %d\n", dmasize);
    }
    printf("read %d, 0x%08X bytes\n", dmasize, dmasize);

    map_base=init_device(dev_num, 0, 0, &fd_bar_0, &fd_bar_1);
    if (fd_bar_1   < 0)  {
        //if (map_base == MAP_FAILED){ //  (void *) -1
        fprintf (stderr,"Error: cannot open device %d \n", dev_num);
        fprintf (stderr," errno = %i\n",errno);
        printf ("open error : %s\n", strerror(errno));
        exit(1);
    }
    printf("CR: 0x%0X\n", read_control_reg(map_base));
    pDevice = (shapi_device_info *) map_base;
    printf("SHAPI VER: 0x%X\n", pDevice->SHAPI_VERSION);
    pModule = (shapi_module_info *) (map_base +
            pDevice->SHAPI_FIRST_MODULE_ADDRESS);
    printf("SHAPI MOD VER: 0x%X\n", pModule->SHAPI_VERSION);
    write_control_reg(map_base, 0);
    trigOff32 = ( (2000U << 16) & 0xFFFF0000) | (-2000 & 0xFFFF);
    printf("TRIG0: 0x%0X\n",trigOff32 );
    write_fpga_reg(map_base, TRIG0_REG_OFF, trigOff32);
    trigOff32 = ( (9000U << 16) & 0xFFFF0000) | (-6000 & 0xFFFF);
    printf("TRIG1: 0x%0X\n",trigOff32 );
    write_fpga_reg(map_base, TRIG1_REG_OFF, trigOff32);
    trigOff32 = ( (4000U << 16) & 0xFFFF0000) | (-9000 & 0xFFFF);
    write_fpga_reg(map_base, TRIG2_REG_OFF, trigOff32);
    printf("TRIG2: 0x%0X\n", read_fpga_reg(map_base, TRIG2_REG_OFF));
    printf("DLY 0x%0X\n", read_fpga_reg(map_base, PULSE_DLY_REG_OFF));
    write_fpga_reg(map_base, PARAM_M_REG_OFF, 0x00030000);
    printf("PMUL: 0x%0X\n", read_fpga_reg(map_base, PARAM_M_REG_OFF));
    write_fpga_reg(map_base, PARAM_OFF_REG_OFF, 0x0A290001);
    //write_fpga_reg(map_base, TRIG0_REG_OFF, (2000 << 16));
//    write_fpga_reg(map_base, TRIG0_REG_OFF, 2000);

    acqData = (short *) malloc(dmasize);
//    i = 0;
    control_reg = read_control_reg(map_base);
    control_reg |= (1<<ACQE_BIT);
  
    write_control_reg(map_base, control_reg);
    //loss_hits = 0;

        printf("CR: 0x%0X, mb:%p\n", read_control_reg(map_base), map_base);
    // ADC board is armed and waiting for trigger (hardware or software)
    if (!hw_trigger)
    {
        printf("FPGA Status: 0x%.8X\n", read_status_reg(map_base));
        printf("software trigger mode active\n");
        getkey(); // Press any key to start the acquisition
        // Start the dacq with a software trigger
        fpga_soft_trigger(map_base);
        //	    rc = ioctl(fd, PCIE_ADC_IOCG_STATUS, &tmp);
        printf("CR: 0x%0X, mb:%p\n", read_control_reg(map_base), map_base);
        printf("FPGA Status: 0x%.8X\n", read_status_reg(map_base));
        // Check status to see if everything is okay (optional)
    }
    else
        printf("hardware trigger mode active\n");

    fflush(stdout);
    //while (loops++ < N_PACKETS) {

    rc = 0;

    rc = read(fd_bar_1, acqData, dmasize); // loop until there is something to read.
    usleep(10);
    dly = read_fpga_reg(map_base, PULSE_DLY_REG_OFF);
    write_control_reg(map_base, 0);  // stop board
    //    rc = read(fd, &acqData[fposition], DMA_SIZE); // loop until there is something to read.
    if (close(fd_bar_1)) {

        printf("Error closing : %s\n", strerror(errno));
    }
    // Check status to see if everything is okay (optional)
    /*rc = ioctl(fd, PCIE_ADC_IOCG_STATUS, &tmp);*/
    rc = read_status_reg(map_base);

//    save_to_disk(dmasize, acqData);
    save_to_bin(dmasize, acqData);

    printf("Streaming off - FPGA Status: 0x%.8X, Delay = 0x%08X, %d \n", rc, dly, dly >>16);
    close(fd_bar_0);
    close(fd_bar_1);
    free(acqData);
    return 0;
    }
