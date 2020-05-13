#include "stm32f4xx.h"
#include "stm32f4_discovery.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4x7_eth_bsp.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include <string.h>
#include <stdio.h>

void LwIP_Init(void);

void vTaskLED1(void *pvParameters) {
    for (;;) {
        STM_EVAL_LEDToggle(LED3);
        vTaskDelay(500);
    }
}

void vTaskLED2(void *pvParameters) {
    for (;;) {
        STM_EVAL_LEDToggle(LED4);
        vTaskDelay(321);
    }
}

void vNetTask(void *pvParameters) {
  struct netconn * nc = (struct netconn *)pvParameters;
  struct netbuf * nb;
  char * buffer = pvPortMalloc(256);
  uint16_t len;
  sprintf(buffer, "Hello from STM32F407VG!\r\n");
  netconn_write(nc,buffer,strlen(buffer), NETCONN_COPY);
  for (;;) {
    netconn_recv(nc, &nb);
    len = netbuf_len(nb);
    netbuf_copy(nb, buffer, len);
    netbuf_delete(nb);
    buffer[len] = 0;
    sprintf(buffer, "%s",buffer);
    netconn_write(nc,buffer,strlen(buffer), NETCONN_COPY);
    vTaskDelay(1);
  }
}

void vTaskDef(void *pvParameters) {
    struct netconn * nc;
    struct netconn * in_nc;
    volatile err_t res;
  /* Initilaize the LwIP stack */
    LwIP_Init();
    nc = netconn_new(NETCONN_TCP);
    if(nc == NULL)
    {
      while(1) vTaskDelay(1);
    }
    res = netconn_bind(nc, IP_ADDR_ANY, 2000);      
    if(res != 0)
    {
      while(1) vTaskDelay(1);
    }
    res = netconn_listen(nc);
    if(res != 0)
    {
      while(1) vTaskDelay(1);
    }
    for (;;) {
        res = netconn_accept(nc, &in_nc);
        STM_EVAL_LEDToggle(LED5);
        xTaskCreate(vNetTask, "NetTask", configMINIMAL_STACK_SIZE, (void*)in_nc, 2, ( TaskHandle_t * ) NULL);
        vTaskDelay(1);
    }
}

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/dhcp.h"
#include "ethernetif.h"
#include "main.h"
#include "tcpip.h"
#include "netif.h"
#include <stdio.h>

struct netif xnetif; /* network interface structure */

void LwIP_Init(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

  /* Create tcp_ip stack thread */
  tcpip_init( NULL, NULL );	

  /* IP address setting & display on STM32_evalboard LCD*/
#ifdef USE_DHCP
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;
#else
  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);
#endif

  /* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
            struct ip_addr *netmask, struct ip_addr *gw,
            void *state, err_t (* init)(struct netif *netif),
            err_t (* input)(struct pbuf *p, struct netif *netif))
    
   Adds your network interface to the netif_list. Allocate a struct
  netif and pass a pointer to this structure as the first argument.
  Give pointers to cleared ip_addr structures when using DHCP,
  or fill them with sane numbers otherwise. The state pointer may be NULL.

  The init function pointer must point to a initialization function for
  your ethernet netif interface. The following code illustrates it's use.*/

  netif_add(&xnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

 /*  Registers the default network interface. */
  netif_set_default(&xnetif);

 /*  When the netif is fully configured this function must be called.*/
  netif_set_up(&xnetif); 
}


int main()
{
    STM_EVAL_LEDInit(LED3);
    STM_EVAL_LEDInit(LED4);
    STM_EVAL_LEDInit(LED5);
    
    /* configure Ethernet (GPIOs, clocks, MAC, DMA) */ 
    ETH_BSP_Config();

       
    xTaskCreate(vTaskLED1, "LED1", configMINIMAL_STACK_SIZE, NULL, 2, ( TaskHandle_t * ) NULL);
    xTaskCreate(vTaskLED2, "LED2", configMINIMAL_STACK_SIZE, NULL, 2, ( TaskHandle_t * ) NULL);
    xTaskCreate(vTaskDef, "TaskDef", configMINIMAL_STACK_SIZE, NULL, 2, ( TaskHandle_t * ) NULL);

    vTaskStartScheduler();
    return 0;
}
