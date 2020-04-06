/*********************************************************************
 *        _       _         _
 *  _ __ | |_  _ | |  __ _ | |__   ___
 * | '__|| __|(_)| | / _` || '_ \ / __|
 * | |   | |_  _ | || (_| || |_) |\__ \
 * |_|    \__|(_)|_| \__,_||_.__/ |___/
 *
 * www.rt-labs.com
 * Copyright 2018 rt-labs AB, Sweden.
 *
 * This software is dual-licensed under GPLv3 and a commercial
 * license. See the file LICENSE.md distributed with this software for
 * full license information.
 ********************************************************************/

#include <kern.h>
#include <uassert.h>
#include <eth/dwmac1000.h>
#include <dev.h>

#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/sys.h>
#include <lwip/stats.h>
#include <lwip/tcpip.h>
#include <lwip/opt.h>

#include <netif/etharp.h>

#include <string.h>

#include "osal.h"

/* Align DMA buffer with lwIP POOL buf size
 * One RX DMA buffer will fit in one POOL buf.
 */
#define BUFFER_SIZE PBUF_POOL_BUFSIZE
COMPILETIME_ASSERT ((BUFFER_SIZE % 4) == 0);

#define RX_MAX_BUFFERS 10
#define TX_MAX_BUFFERS 10

#undef RTK_DEBUG                /* Print debugging info? */
#undef DEBUG_DATA               /* Print packets? */

#ifdef RTK_DEBUG
#define DPRINT(...) rprintp ("dwmac1000: "__VA_ARGS__)
#define DEBUG_ASSERT(expression)    ASSERT(expression)
#else
#define DPRINT(...)
#define DEBUG_ASSERT(expression)
#endif  /* DEBUG */

typedef struct eth_mac
{
   uint32_t cr;
   uint32_t ffr;
   uint32_t hthr;
   uint32_t htlr;
   uint32_t miiar;
   uint32_t miidr;
   uint32_t fcr;
   uint32_t vlantr;
   uint32_t reserved0[2];
   uint32_t rwuffr;
   uint32_t pmtcsr;
   uint32_t reserved1[2];
   uint32_t macsr;
   uint32_t macimr;
   uint32_t maca0hr;
   uint32_t maca0lr;
   uint32_t maca1hr;
   uint32_t maca1lr;
   uint32_t maca2hr;
   uint32_t maca2lr;
   uint32_t maca3hr;
   uint32_t maca3lr;
} eth_mac_t;

COMPILETIME_ASSERT (offsetof(eth_mac_t, maca3lr) == 0x5c);

#define CR_RE   BIT(2)
#define CR_TE   BIT(3)
#define CR_DC   BIT(4)
#define CR_APCS BIT(7)
#define CR_RD   BIT(9)
#define CR_IPCO BIT(10)
#define CR_DM   BIT(11)
#define CR_LM   BIT(12)
#define CR_ROD  BIT(13)
#define CR_FES  BIT(14)
#define CR_CSD  BIT(16)
#define CR_JD   BIT(22)
#define CR_WD   BIT(23)
#define CR_CSTF BIT(25)

/* Frame Filter Register */
#define FFR_PM  BIT(4)        /* Pass All Multicast */

#define MIIAR_MB    BIT(0)
#define MIIAR_MW    BIT(1)
#define MIIAR_CR42  (0 << 2)
#define MIIAR_CR62  (1 << 2)
#define MIIAR_CR16  (2 << 2)
#define MIIAR_CR26  (3 << 2)
#define MIIAR_CR102 (4 << 2)

/* Ethernet MAC Management Counter registers */
typedef struct eth_mmc
{
   uint32_t cr;         /* Control */
   uint32_t rir;        /* Receive Interrupt */
   uint32_t tir;        /* Transmit Interrupt  */
   uint32_t rimr;       /* Receive Interrupt Mask */
   uint32_t timr;       /* Transmit Interrupt Mask */
   uint32_t reserved0[0x3b];
   uint32_t ipc_rcoimr; /* Receive Checksum Offload Interrupt Mask*/
} eth_mmc_t;

COMPILETIME_ASSERT (offsetof(eth_mmc_t, timr) == 0x10);
COMPILETIME_ASSERT (offsetof(eth_mmc_t, ipc_rcoimr) == 0x100);

typedef struct eth_dma
{
   uint32_t bmr;
   uint32_t tpdr;
   uint32_t rpdr;
   uint32_t rdlar;
   uint32_t tdlar;
   uint32_t sr;
   uint32_t omr;
   uint32_t ier;
   uint32_t mfbocr;
   uint32_t rswtr;
   uint32_t reserved0[8];
   uint32_t chtdr;
   uint32_t chrdr;
   uint32_t chtbar;
   uint32_t chrbar;
} eth_dma_t;

#define BMR_SR   BIT(0)
#define BMR_DA   BIT(1)
#define BMR_EDFE BIT(7)
#define BMR_FB   BIT(16)
#define BMR_USP  BIT(23)
#define BMR_FPM  BIT(24)
#define BMR_AAB  BIT(25)
#define BMR_MB   BIT(26)

#define SR_TS          BIT(0)
#define SR_TPSS        BIT(1)
#define SR_TBUS        BIT(2)
#define SR_TJTS        BIT(3)
#define SR_ROS         BIT(4)
#define SR_TUS         BIT(5)
#define SR_RS          BIT(6)
#define SR_RBUS        BIT(7)
#define SR_RPSS        BIT(8)
#define SR_RWTS        BIT(9)
#define SR_ETS         BIT(10)
#define SR_FBES        BIT(13)
#define SR_ERS         BIT(14)
#define SR_AIS         BIT(15)
#define SR_NIS         BIT(16)
#define SR_TPS         (7 << 20)
#define SR_TPS_STALLED (6 << 20)
#define SR_MMCS        BIT(27)
#define SR_PMTS        BIT(28)
#define SR_TSTS        BIT(29)

#define OMR_SR     BIT(1)
#define OMR_OSF    BIT(2)
#define OMR_FUGF   BIT(6)
#define OMR_FEF    BIT(7)
#define OMR_ST     BIT(13)
#define OMR_FTF    BIT(20)
#define OMR_TSF    BIT(21)
#define OMR_DFRF   BIT(24)
#define OMR_RSF    BIT(25)
#define OMR_DTCEFD BIT(26)

#define IER_TIE   BIT(0)
#define IER_TPSIE BIT(1)
#define IER_TBUIE BIT(2)
#define IER_TJTIE BIT(3)
#define IER_ROIE  BIT(4)
#define IER_TUIE  BIT(5)
#define IER_RIE   BIT(6)
#define IER_RBUIE BIT(7)
#define IER_RPSIE BIT(8)
#define IER_RWTIE BIT(9)
#define IER_ETIE  BIT(10)
#define IER_FBEIE BIT(13)
#define IER_ERIE  BIT(14)
#define IER_AISE  BIT(15)
#define IER_NISE  BIT(16)

typedef struct dma_descriptor
{
   uint32_t des0;
   uint32_t des1;
   uint8_t * buff;
   volatile struct dma_descriptor * next;
   struct pbuf * pbuf;
} dma_descriptor_t;

#define xDES0_OWN BIT(31)        /* Owned by DMA engine */
#define TDES0_IC  BIT(30)        /* Interrupt on completion */
#define TDES0_LS  BIT(29)        /* Last segment */
#define TDES0_FS  BIT(28)        /* First segment */
#define TDES0_TCH BIT(20)        /* Chained descriptors */

#define RDES0_AFM  BIT (30)       /* Failed address filter */
#define RDES0_ES   BIT (15)       /* Error summary */
#define RDES0_VLAN BIT (10)       /* VLAN frame */
#define RDES0_FS   BIT (9)        /* First segment */
#define RDES0_LS   BIT (8)        /* Last segment */

#define RDES1_RCH BIT(14)        /* Chained descriptors */

typedef struct dwmac1000
{
   drv_t drv;

   irq_t irq;
   mtx_t * mtx_tx;
   task_t * tRcv;
   phy_t * phy;
   volatile eth_mac_t * mac;
   volatile eth_mmc_t * mmc;
   volatile eth_dma_t * dma;

   /* Receive and transmit DMA descriptors */
   volatile dma_descriptor_t rx[RX_MAX_BUFFERS];
   volatile dma_descriptor_t tx[TX_MAX_BUFFERS];
   volatile dma_descriptor_t *pRx;
   volatile dma_descriptor_t *pTx;

   /* MIIAR CR divider */
   uint32_t cr;
} dwmac1000_t;

static os_eth_callback_t* input_rx_hook = NULL;
static void* input_rx_arg = NULL;

#ifdef DEBUG_DATA
#include <ctype.h>
static void dwmac1000_pbuf_dump (struct pbuf *p)
{
   int i, j, n;
   char s[80];

   for (i = 0; i < p->len; i += 16)
   {
      n = 0;
      for (j = 0; j < 16 && (i + j) < p->len; j++)
      {
         n += rsnprintf (s + n, sizeof(s) - n, "%02x ",
                         *(uint8_t *)(p->payload + i + j));
      }
      for (; j < 16; j++)
      {
         n += rsnprintf (s + n, sizeof(s) - n, "   ");
      }
      n += rsnprintf (s + n, sizeof(s) - n, "|");
      for (j = 0; j < 16 && (i + j) < p->len; j++)
      {
         uint8_t c = *(uint8_t *)(p->payload + i + j);
         c = (isprint (c)) ? c : '.';
         n += rsnprintf (s + n, sizeof(s) - n, "%c", c);
      }
      n += rsnprintf (s + n, sizeof(s) - n, "|\n");
      DPRINT ("%s", s);
   }
}
#endif  /* DEBUG_DATA */

static uint16_t dwmac1000_read_phy (void * arg, uint8_t address, uint8_t reg)
{
   dwmac1000_t *dwmac1000 = arg;
   volatile eth_mac_t * mac = dwmac1000->mac;
   uint16_t value;

   mac->miiar = (address << 11) | (reg << 6) | MIIAR_MB |
      dwmac1000->cr;
   while (mac->miiar & MIIAR_MB);

   value = mac->miidr;
   DPRINT ("phy_read: %02d = %04x\n", reg, value);
   return value;
}

static void dwmac1000_write_phy (void * arg, uint8_t address, uint8_t reg,
                                uint16_t value)
{
   dwmac1000_t *dwmac1000 = arg;
   volatile eth_mac_t * mac = dwmac1000->mac;

   mac->miidr = value;
   mac->miiar = (address << 11) | (reg << 6) | MIIAR_MW | MIIAR_MB |
      dwmac1000->cr;
   while (mac->miiar & MIIAR_MB);

   DPRINT ("phy_write: %02d = %04x\n", reg, value);
}

static void dwmac1000_hw_init (struct netif *netif, const dwmac1000_cfg_t * cfg)
{
   dwmac1000_t *dwmac1000 = netif->state;
   volatile eth_mac_t * mac = dwmac1000->mac;
   volatile eth_mmc_t * mmc = dwmac1000->mmc;
   volatile eth_dma_t * dma = dwmac1000->dma;
   unsigned int ix;

   /* Software reset */
   dma->bmr = BMR_SR;
   while (dma->bmr & BMR_SR);

   /* Configure MII divider according to HCLK */
   if (cfg->hclk >= 150 * 1000 * 1000)
      dwmac1000->cr = MIIAR_CR102;
   else if (cfg->hclk >= 100 * 1000 * 1000)
      dwmac1000->cr = MIIAR_CR62;
   else if (cfg->hclk >= 60 * 1000 * 1000)
      dwmac1000->cr = MIIAR_CR42;
   else if (cfg->hclk >= 35 * 1000 * 1000)
      dwmac1000->cr = MIIAR_CR26;
   else if (cfg->hclk >= 20 * 1000 * 1000)
      dwmac1000->cr = MIIAR_CR16;

   /* Reset PHY */
   if (dwmac1000->phy->ops->reset)
   {
      dwmac1000->phy->ops->reset (dwmac1000->phy);
   }

   /* Start link autonegotiation */
   DEBUG_ASSERT (dwmac1000->phy->ops->start != NULL);
   dwmac1000->phy->ops->start (dwmac1000->phy);

   /* Enable receiver, transmitter */
   mac->cr = CR_RE | CR_TE;

   /* Set mac address */
   mac->maca0hr =
      netif->hwaddr[5] << 8 |
      netif->hwaddr[4] << 0;
   mac->maca0lr =
      netif->hwaddr[3] << 24 |
      netif->hwaddr[2] << 16 |
      netif->hwaddr[1] << 8  |
      netif->hwaddr[0] << 0;

   /* Enable reception of all broadcast and multicast frames.
    * Enable reception of unicast frames with selected MAC address.
    */
   mac->ffr = FFR_PM;

   /* Disable MMC counter interrupts */
   mmc->rimr = UINT32_MAX;
   mmc->timr = UINT32_MAX;
   mmc->ipc_rcoimr = UINT32_MAX;

   /* Setup Rx descriptors using chained mode */
   for (ix = 0; ix < cfg->rx_buffers; ix++)
   {
      struct pbuf * p = pbuf_alloc(PBUF_RAW, BUFFER_SIZE, PBUF_POOL);
      UASSERT (p != NULL, EMEM);

      dwmac1000->rx[ix].des0 = xDES0_OWN;
      dwmac1000->rx[ix].des1 = RDES1_RCH | BUFFER_SIZE;
      dwmac1000->rx[ix].buff = p->payload;
      dwmac1000->rx[ix].next = &dwmac1000->rx[ix + 1];
      dwmac1000->rx[ix].pbuf = p;
   }
   dwmac1000->rx[ix - 1].next = &dwmac1000->rx[0]; /* Point to first */
   dwmac1000->pRx = &dwmac1000->rx[0];             /* Driver index */

   /* Setup Tx descriptors using chained mode */
   for (ix = 0; ix < cfg->tx_buffers; ix++)
   {
      uint8_t * p = malloc (BUFFER_SIZE);
      UASSERT (p != NULL, EMEM);

      /* des1 will be setup later, before transfer */
      dwmac1000->tx[ix].des0 = TDES0_TCH;
      dwmac1000->tx[ix].buff = p;
      dwmac1000->tx[ix].next = &dwmac1000->tx[ix + 1];
      dwmac1000->tx[ix].pbuf = NULL;
   }
   dwmac1000->tx[ix - 1].next = &dwmac1000->tx[0]; /* Point to first */
   dwmac1000->pTx = &dwmac1000->tx[0];             /* Driver index */

   /* Set descriptor pointers */
   dma->rdlar = (uint32_t)dwmac1000->rx;
   dma->tdlar = (uint32_t)dwmac1000->tx;

   /* Enable Rx interrupt */
   dma->ier = IER_NISE | IER_RIE;

   /* Configure DMA (burst length 1) */
   dma->bmr = BMR_AAB | (1 << 8);

   /* Flush FIFO */
   dma->omr = OMR_FTF;
   while (dma->omr & OMR_FTF);

   /* Start DMA */
   dma->omr = OMR_DTCEFD | OMR_RSF | OMR_TSF | OMR_ST | OMR_SR;
}

/*
 * dwmac1000_hw_transmit_frame():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
static err_t dwmac1000_hw_transmit_frame (struct netif *netif, struct pbuf *p)
{
   dwmac1000_t *dwmac1000 = netif->state;
   volatile eth_dma_t * dma = dwmac1000->dma;
   struct pbuf *q;
   volatile dma_descriptor_t * pTx;
   uint8_t * buffer;

   if (p->tot_len > BUFFER_SIZE)
   {
      DPRINT("transmit_frame: frame is too big\n");
      LINK_STATS_INC (link.drop);
      LINK_STATS_INC(link.lenerr);
      return ERR_OK;
   }

   /* Lock TX handling */
   mtx_lock (dwmac1000->mtx_tx);

#if ETH_PAD_SIZE
   pbuf_header (p, -ETH_PAD_SIZE);  /* drop the padding word */
#endif

   pTx = dwmac1000->pTx;
   buffer = pTx->buff;

   /* Has descriptor been processed? */
   if (pTx->des0 & xDES0_OWN)
   {
      /* No free descriptors, packet dropped */
      LINK_STATS_INC(link.drop);
      goto exit;
   }

   /* Copy frame to DMA buffer */
   for (q = p; q != NULL; q = q->next)
   {
      memcpy (buffer, q->payload, q->len);
      buffer += q->len;

      DPRINT ("out (%d):\n", q->len);
#ifdef DEBUG_DATA
      dwmac1000_pbuf_dump (q);
#endif  /* DEBUG_DATA */
   }

   /* Finalize DMA descriptor. Passes ownership of descriptor to hardware */
   pTx->des1 = p->tot_len;
   pTx->des0 = xDES0_OWN | TDES0_FS | TDES0_LS | TDES0_TCH;

   /* Start Tx engine if stalled */
   dma->sr = SR_TBUS;     /* Clear buffer unavailable */
   dma->tpdr = 1;         /* Demand transmit poll */

   /* Move to next descriptor */
   pTx = pTx->next;
   dwmac1000->pTx = pTx;

   LINK_STATS_INC(link.xmit);

 exit:

#if ETH_PAD_SIZE
   pbuf_header (p, ETH_PAD_SIZE);   /* reclaim the padding word */
#endif

   mtx_unlock (dwmac1000->mtx_tx);
   return ERR_OK;
}

static void dwmac1000_isr (void *arg)
{
   dwmac1000_t *dwmac1000 = (dwmac1000_t *)arg;
   volatile eth_dma_t * dma = dwmac1000->dma;
   uint32_t sr;

   /* Clear status */
   sr = dma->sr;
   dma->sr = sr;

   if(sr & SR_RS)
   {
      /* Packet received, serve in task */
      task_start (dwmac1000->tRcv);

      /* Disable receive interrupt while task start is pending/running */
      dma->ier &= ~IER_RIE;
   }
}

/*
 * dwmac1000_hw_get_received_frame():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 */
static struct pbuf *dwmac1000_hw_get_received_frame (dwmac1000_t *dwmac1000)
{
   struct pbuf *p = NULL;
   struct pbuf *q = NULL;
   uint16_t length;
   volatile dma_descriptor_t * pRx;
   uint32_t des0;

   pRx = dwmac1000->pRx;
   des0 = pRx->des0;

   /* Ignore packets with errors */
   if ((des0 & (RDES0_AFM | RDES0_ES)) != 0)
      goto done;

   /* Ignore incomplete packets */
   if ((des0 & (RDES0_FS | RDES0_LS)) != (RDES0_FS | RDES0_LS))
      goto done;

   /* Fetch received pbuf */
   p = pRx->pbuf;
   p->payload = pRx->buff;
   length = (des0 >> 16) & 0x3FFF;
   length -= 4;                 /* Drop CRC */
   p->len = p->tot_len = length;

   DPRINT ("in (%d):\n", p->len);
#ifdef DEBUG_DATA
   dwmac1000_pbuf_dump (q);
#endif  /* DEBUG_DATA */

#if ETH_PAD_SIZE
#error "ETH_PAD_SIZE not supported"
#endif

   /* Allocate a pbuf from the pool to replace the one currently use by DMA. */
   q = pbuf_alloc (PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);

   if (q != NULL)
   {
      DEBUG_ASSERT(q->len == q->tot_len);
      pRx->buff = q->payload;
      pRx->pbuf = q;
      LINK_STATS_INC (link.recv);
   }
   else
   {
      p = NULL;
      LINK_STATS_INC (link.memerr);
      LINK_STATS_INC (link.drop);
   }

 done:
   /* Assign descriptor back to DMA engine */
   pRx->des0 = xDES0_OWN;

   if (dwmac1000->dma->sr & SR_RBUS)
   {
      /* Restart stalled DMA */
      dwmac1000->dma->rpdr = 1;
   }

   return p;
}

/*
 * dwmac1000_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function
 * dwmac1000_hw_get_received_frame() that should handle the actual
 * reception of bytes from the network interface. Then the type of the
 * received packet is determined and the appropriate input function is
 * called.
 *
 */
static void dwmac1000_input (dwmac1000_t *dwmac1000, struct netif *netif)
{
   int handled = 0;
   struct eth_hdr *ethhdr;
   struct pbuf *p;

   /* Move received packet into a new pbuf */
   p = dwmac1000_hw_get_received_frame (dwmac1000);
   if (p == NULL)
   {
      /* No packet could be read, silently ignore this */
      return;
   }

   /* points to packet payload, which starts with an Ethernet header */
   ethhdr = p->payload;

   /* Pass pbuf to rx hook if set */
   if (input_rx_hook != NULL)
   {
      handled = input_rx_hook(input_rx_arg, p);
      if (handled != 0)
      {
         return;
      }
      /* Else pass it to lwIP */
   }

   switch (htons (ethhdr->type))
   {
   case ETHTYPE_VLAN:
   case ETHTYPE_IP:
      /* Fall-through */
   case ETHTYPE_ARP:
      if (netif->input (p, netif) != (err_t)ERR_OK)
      {
         LWIP_DEBUGF (NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
         pbuf_free (p);
      }
      break;
   default:
      pbuf_free (p);
      break;
   }
}

/*
 * dwmac1000_receiver():
 *
 * NOTE: It is not thread-safe and expected to only be called from
 * EthRcv.
 */
static void dwmac1000_receiver (void *arg)
{
   struct netif *netif = (struct netif *)arg;
   dwmac1000_t *dwmac1000 = netif->state;

   for (;;)
   {
      /* Wait for RCV event */
      task_stop();

      /* Several packets may have arrived, check all that DMA engine
       * has released
       */
      while ((dwmac1000->pRx->des0 & xDES0_OWN) == 0)
      {
         /* Get one packet */
         dwmac1000_input (dwmac1000, netif);

         /* Move to next descriptor */
         dwmac1000->pRx = dwmac1000->pRx->next;
      }

      /* Re-enable receive interrupt */
      dwmac1000->dma->ier |= IER_RIE;
   }
}

static void dwmac1000_hotplug (drv_t * drv, dev_state_t state)
{
   dwmac1000_t * dwmac1000 = (dwmac1000_t *)drv;
   volatile eth_mac_t * mac = dwmac1000->mac;
   uint8_t link_state;
   uint32_t cr;

   if (state == StateDetaching)
   {
      dev_set_state (drv, StateDetached);
      return;
   }

   /* Set duplex mode according to link state */
   link_state = dwmac1000->phy->ops->get_link_state (dwmac1000->phy);

   cr = mac->cr;
   cr &= ~(CR_FES | CR_DM);
   if (link_state & PHY_LINK_FULL_DUPLEX)
   {
      cr |= CR_DM;
   }
   if (link_state & PHY_LINK_100MBIT)
   {
      cr |= CR_FES;
   }
   mac->cr = cr;

   dev_set_state (drv, StateAttached);
}

static dev_state_t dwmac1000_probe (drv_t * drv)
{
   dwmac1000_t *dwmac1000 = (dwmac1000_t *)drv;
   uint8_t link_state;

   link_state = dwmac1000->phy->ops->get_link_state (dwmac1000->phy);

   return (link_state & PHY_LINK_OK) ? StateAttached : StateDetached;
}

int eth_ioctl (drv_t * drv, void * arg, int req, void * param)
{
   int status = EARG;

   if (req == IOCTL_NET_SET_RX_HOOK)
   {
      input_rx_hook = (os_eth_callback_t*)param;
      input_rx_arg = arg;
      return 0;
   }
   else
   {
      UASSERT(0, EARG);
   }

   return status;
}

static const drv_ops_t dwmac1000_ops =
{
   .open    = NULL,
   .read    = NULL,
   .write   = NULL,
   .close   = NULL,
   .ioctl   = NULL,
   .hotplug = dwmac1000_hotplug,
};

drv_t * dwmac1000_init (const char * name, const dwmac1000_cfg_t * cfg,
                       struct netif * netif, phy_t * phy)
{
   dwmac1000_t *dwmac1000;

   UASSERT (cfg->hclk >= 20 * 1000 * 1000, EARG);
   UASSERT (cfg->rx_buffers <= RX_MAX_BUFFERS, EARG);
   UASSERT (cfg->rx_buffers < PBUF_POOL_SIZE, EARG);
   UASSERT (cfg->tx_buffers <= TX_MAX_BUFFERS, EARG);

   dwmac1000 = malloc (sizeof (dwmac1000_t));
   UASSERT (dwmac1000 != NULL, EMEM);

   /* Initialise netif */
   netif->state      = dwmac1000;
   netif->name[0]    = 'e';
   netif->name[1]    = 'n';
   netif->output     = etharp_output;
   netif->linkoutput = dwmac1000_hw_transmit_frame;
   netif->mtu = 1500;   /* maximum transfer unit */
   netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
   netif->hwaddr_len = ETHARP_HWADDR_LEN;
   memcpy (netif->hwaddr, cfg->mac_address, sizeof(netif->hwaddr));

   /* Initialise driver state */
   dwmac1000->phy = phy;
   dwmac1000->irq = cfg->irq;
   dwmac1000->mtx_tx = mtx_create();
   dwmac1000->mac = (eth_mac_t *)cfg->base;
   dwmac1000->mmc = (eth_mmc_t *)(cfg->base + 0x100);
   dwmac1000->dma = (eth_dma_t *)(cfg->base + 0x1000);

   /* Create receive task */
   dwmac1000->tRcv = task_spawn ("EthRcv",
                                 dwmac1000_receiver,
                                 cfg->rx_task_prio,
                                 cfg->rx_task_stack + 256,  // Bjarne
                                 netif);

   /* Initialise PHY */
   dwmac1000->phy->arg = dwmac1000;
   if (dwmac1000->phy->read == NULL)
      dwmac1000->phy->read  = dwmac1000_read_phy;
   if (dwmac1000->phy->write == NULL)
      dwmac1000->phy->write = dwmac1000_write_phy;

   /* Initialise hardware */
   dwmac1000_hw_init (netif, cfg);
   int_connect (dwmac1000->irq, dwmac1000_isr, dwmac1000);
   int_enable (dwmac1000->irq);

   /* Install device driver */
   dwmac1000->drv.ops = &dwmac1000_ops;
   dev_install ((drv_t *)dwmac1000, name);

   /* Register a function to check for established link */
   dev_set_probe (name, dwmac1000_probe);

   return (drv_t *)dwmac1000;
}
