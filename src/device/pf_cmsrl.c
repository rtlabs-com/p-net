#include <string.h>
#include <pf_includes.h>

static void ar_set_update_state (pnet_t *, pf_ar_set_t *);

/* Convenience functions */

static void set_ar_input_data_status (
   pnet_t * net,
   pf_ar_t * ar,
   bool primary,
   bool redundancy)
{
   int n;

   for (n = 0; n < ar->io_ar->nbr_iocrs; ++n)
   {
      if (ar->io_ar->iocrs[n].param.iocr_type == PF_IOCR_TYPE_INPUT)
      {
         uint32_t crep = ar->io_ar->iocrs[n].crep;

         pf_ppm_set_data_status_state (net, ar, crep, primary);
         pf_ppm_set_data_status_redundancy (net, ar, crep, redundancy);
      }
   }
}

static bool last_ar_of_ar_set (pf_ar_t * ar)
{
   int n;
   pf_ar_set_t * ar_set;

   ar_set = ar->ar_set;
   if (ar_set == NULL)
   {
      return false;
   }
   for (n = 0; n < ar_set->size; ++n)
   {
      if (ar_set->contains_ar[n] && (ar_set->ar[n] != ar))
      {
         return false;
      }
   }
   return true;
}

/* CMSRL_AL functions */

bool pf_cmsrl_al_issue_alarm_req (
   pnet_t * net,
   pf_ar_set_t * ar_set,
   int16_t prio,
   pf_alarm_data_t * alarm_data)
{
   pf_apmx_t * apmx;

   os_mutex_lock (ar_set->lock);
   switch (ar_set->cmsrl_al[prio].state)
   {
   case PF_CMSRL_AL_STATE_TRANSMIT:
      apmx = &ar_set->primary_ar->io_ar->apmx[prio];
      memcpy (
         ar_set->cmsrl_al[prio].alarm.data,
         alarm_data,
         sizeof (*alarm_data));
      ar_set->cmsrl_al[prio].alarm.stored = true;
      pf_alarm_forward_alarm_req (net, apmx, ar_set->cmsrl_al[prio].alarm.data);
      os_mutex_unlock (ar_set->lock);
      LOG_DEBUG (
         PF_ALARM_LOG,
         "CMSRL_AL(%d) Store and send [%s]\n",
         prio,
         __FUNCTION__);
      return true;
      break;
   case PF_CMSRL_AL_STATE_STORE:
      memcpy (
         ar_set->cmsrl_al[prio].alarm.data,
         alarm_data,
         sizeof (*alarm_data));
      ar_set->cmsrl_al[prio].alarm.stored = true;
      os_mutex_unlock (ar_set->lock);
      LOG_DEBUG (PF_ALARM_LOG, "CMSRL_AL(%d) Store [%s]\n", prio, __FUNCTION__);
      return true;
      break;
   default:
      os_mutex_unlock (ar_set->lock);
      return false;
      break;
   }
}

void pf_cmsrl_al_forward_alarm_cnf (
   pf_ar_set_t * ar_set,
   bool high_prio,
   pnet_pnio_status_t const * pnio_status)
{
   uint8_t channel = (high_prio ? 1 : 0);

   os_mutex_lock (ar_set->lock);
   if (ar_set->cmsrl_al[channel].state == PF_CMSRL_AL_STATE_TRANSMIT)
   {
      if (
         (pnio_status->error_code == PNET_ERROR_CODE_ALARM_ACK) &&
         (pnio_status->error_decode == PNET_ERROR_DECODE_PNIO) &&
         (pnio_status->error_code_1 == PNET_ERROR_CODE_1_ALARM_ACK) &&
         (pnio_status->error_code_2 ==
          PNET_ERROR_CODE_2_ALARM_ACK_IOCARSR_BACKUP))
      {
         ar_set->cmsrl_al[channel].state = PF_CMSRL_AL_STATE_STORE;
         LOG_DEBUG (
            PF_ALARM_LOG,
            "CMSRL_AL(%d) TRANSMIT -> STORE [%s]\n",
            channel,
            __FUNCTION__);
      }
      else
      {
         LOG_DEBUG (
            PF_ALARM_LOG,
            "CMSRL_AL(%d) Clearing stored [%s]\n",
            channel,
            __FUNCTION__);
         ar_set->cmsrl_al[channel].alarm.stored = false;
      }
   }
   os_mutex_unlock (ar_set->lock);
}

static void pf_cmsrl_al_ar_set_state_changed (pnet_t * net, pf_ar_set_t * ar_set)
{
   int i;

   i = PF_ALARM_NUMBER_OF_PRIORITY_LEVELS;
   while (i--)
   {
      switch (ar_set->cmsrl_al[i].state)
      {
      case PF_CMSRL_AL_STATE_STARTUP:
         if (ar_set->state == PF_AR_SET_STATE_PRIMARY_EXIST)
         {
            ar_set->cmsrl_al[i].state = PF_CMSRL_AL_STATE_TRANSMIT;
            LOG_DEBUG (
               PF_ALARM_LOG,
               "CMSRL_AL(%d) STARTUP -> TRANSMIT [%s]\n",
               i,
               __FUNCTION__);
         }
         break;
      case PF_CMSRL_AL_STATE_TRANSMIT:
         if (ar_set->state == PF_AR_SET_STATE_PRIMARY_LOST)
         {
            ar_set->cmsrl_al[i].state = PF_CMSRL_AL_STATE_STORE;
            LOG_DEBUG (
               PF_ALARM_LOG,
               "CMSRL_AL(%d) TRANSMIT -> STORE [%s] stored=%s\n",
               i,
               __FUNCTION__,
               ar_set->cmsrl_al[i].alarm.stored ? "YES" : "NO");
         }
         break;
      case PF_CMSRL_AL_STATE_STORE:
         if (ar_set->state == PF_AR_SET_STATE_PRIMARY_EXIST)
         {
            pf_apmx_t * apmx = &ar_set->primary_ar->io_ar->apmx[i];

            ar_set->cmsrl_al[i].state = PF_CMSRL_AL_STATE_TRANSMIT;
            LOG_DEBUG (
               PF_ALARM_LOG,
               "CMSRL_AL(%d) STORE -> TRANSMIT [%s] stored=%s\n",
               i,
               __FUNCTION__,
               ar_set->cmsrl_al[i].alarm.stored ? "YES" : "NO");
            if (ar_set->cmsrl_al[i].alarm.stored)
            {
               pf_alarm_forward_alarm_req (
                  net,
                  apmx,
                  ar_set->cmsrl_al[i].alarm.data);
            }
         }
         break;
      default:
         break;
      }
   }
}

/* CMSRL functions */

static void pf_cmsrl_rdht_expired (
   pnet_t * net,
   void * arg,
   uint32_t current_time)
{
   pf_ar_set_t * ar_set = (pf_ar_set_t *)arg;
   pf_ar_t ** ar = ar_set->ar;
   int n;

   pf_scheduler_reset_handle (ar_set->rdht);

   for (n = 0; n < ar_set->size; ++n)
   {
      if (ar_set->contains_ar[n])
      {
         switch (ar[n]->cmsrl.state)
         {
         case PF_CMSRL_STATE_BACKUP:
            ar[n]->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
            ar[n]->err_code = PNET_ERROR_CODE_2_ABORT_IOCARSR_RDHT_EXPIRED;

            ar[n]->cmsrl.state = PF_CMSRL_STATE_IDLE;
            (void)pf_cmdev_state_ind (net, ar[n], PNET_EVENT_ABORT);
            break;
         case PF_CMSRL_STATE_PRIMARY: /* Fall through */
         case PF_CMSRL_STATE_DBACKUP:
            ar[n]->ar_state = PF_AR_STATE_BACKUP;

            ar[n]->err_cls = PNET_ERROR_CODE_1_RTA_ERR_CLS_PROTOCOL;
            ar[n]->err_code = PNET_ERROR_CODE_2_ABORT_IOCARSR_RDHT_EXPIRED;

            ar[n]->cmsrl.state = PF_CMSRL_STATE_IDLE;
            (void)pf_cmdev_state_ind (net, ar[n], PNET_EVENT_ABORT);
            break;
         default:
            break;
         }
      }
   }
}

static void pf_cmsrl_start_rdht (
   pnet_t * net,
   pf_ar_set_t * ar_set,
   uint16_t rdhf)
{
   (void)pf_scheduler_restart (
      net,
      (rdhf * 1000),
      pf_cmsrl_rdht_expired,
      ar_set,
      ar_set->rdht);
}

static void pf_cmsrl_stop_rdht (pnet_t * net, pf_ar_set_t * ar_set)
{
   pf_scheduler_remove_if_running (net, ar_set->rdht);
}

static void pf_cmsrl_state_ind_primary (pnet_t * net, pf_ar_t * primary_ar)
{
   pf_ar_set_t * ar_set = primary_ar->ar_set;
   pf_ar_t ** ar = ar_set->ar;
   int n;

   for (n = 0; n < ar_set->size; ++n)
   {
      if (ar_set->contains_ar[n])
      {
         if (ar[n] != primary_ar)
         {
            if (ar[n]->cmsrl.state == PF_CMSRL_STATE_PRIMARY)
            {
               /* CleanAlarmChannels(this); */
               ar[n]->ar_state = PF_AR_STATE_BACKUP;
               /* UpdateDataTransferControl(); */
               /* PrimaryFault: */
               set_ar_input_data_status (net, ar[n], true, true);
               ar[n]->cmsrl.state = PF_CMSRL_STATE_DBACKUP;
            }
         }
      }
   }
}

void pf_cmsrl_cmdev_state_ind (
   pnet_t * net,
   pf_ar_t * ar,
   pnet_event_values_t event)
{
   os_mutex_lock (ar->ar_set->lock);
   switch (ar->cmsrl.state)
   {
   case PF_CMSRL_STATE_IDLE:
      if (event == PNET_EVENT_DATA)
      {
         ar->ar_state = PF_AR_STATE_BACKUP;
         ar->cmsrl.state = PF_CMSRL_STATE_BACKUP;
      }
      break;
   case PF_CMSRL_STATE_PRIMARY: /* Fall through */
   case PF_CMSRL_STATE_DBACKUP:
      if (event == PNET_EVENT_ABORT)
      {
         if (last_ar_of_ar_set (ar))
         {
            pf_cmsrl_stop_rdht (net, ar->ar_set);
         }
         ar->cmsrl.state = PF_CMSRL_STATE_IDLE;
         ar_set_update_state (net, ar->ar_set);
      }
      break;
   default:
      break;
   }
   os_mutex_unlock (ar->ar_set->lock);
}

static void pf_cmsrl_ar_set_state_changed (pnet_t * net, pf_ar_set_t * ar_set)
{
   pf_ar_t ** ar = ar_set->ar;
   int n;

   for (n = 0; n < ar_set->size; ++n)
   {
      if (ar_set->contains_ar[n])
      {
         if (ar[n]->cmsrl.state == PF_CMSRL_STATE_BACKUP)
         {
            if (ar_set->state == PF_AR_SET_STATE_PRIMARY_EXIST)
            {
               /* BackupAcknowledge: */
               set_ar_input_data_status (net, ar[n], false, false);
            }
            else if (ar_set->state == PF_AR_SET_STATE_PRIMARY_LOST)
            {
               /* PrimaryMissing: */
               set_ar_input_data_status (net, ar[n], false, true);
            }
         }
      }
   }
}

void pf_cmsrl_cpm_new_data_ind (
   pnet_t * net,
   pf_ar_t * ar,
   uint16_t crep,
   bool primary)
{
   pf_ar_set_t * ar_set;

   if (ar->ar_param.ar_type != PF_ART_IOCAR_SR)
   {
      return;
   }
   ar_set = ar->ar_set;
   os_mutex_lock (ar_set->lock);
   switch (ar->cmsrl.state)
   {
   case PF_CMSRL_STATE_BACKUP:
      if (primary)
      {
         /* PrepareDataBuffers (PRIMARY) */
         /* AssignAlarmChannels () */
         pf_cmsrl_start_rdht (
            net,
            ar_set,
            ar->sr.info.redundancy_data_hold_factor);
         ar->ar_state = PF_AR_STATE_PRIMARY;
         /* UpdateDataTransferControl () */
         pf_cmsrl_state_ind_primary (net, ar);
         /* PrimaryAcknowledge: */
         set_ar_input_data_status (net, ar, true, false);
         ar->cmsrl.state = PF_CMSRL_STATE_PRIMARY;
         ar_set_update_state (net, ar_set);
      }
      break;
   case PF_CMSRL_STATE_PRIMARY:
      if (primary)
      {
         pf_cmsrl_start_rdht (
            net,
            ar_set,
            ar->sr.info.redundancy_data_hold_factor);
      }
      else
      {
         /* PrepareDataBuffers (BACKUP) */
         /* CleanAlarmChannels () */
         ar->ar_state = PF_AR_STATE_BACKUP;
         /* UpdateDataTransferControl () */
         /* ar->cmsrl.state = PF_CMSRL_STATE_WBACKUP; */
         ar_set_update_state (net, ar_set);
         ar->cmsrl.state = PF_CMSRL_STATE_BACKUP;
         set_ar_input_data_status (
            net,
            ar,
            false,
            (ar_set->state == PF_AR_SET_STATE_PRIMARY_LOST));
      }
      break;
   case PF_CMSRL_STATE_DBACKUP:
      if (!primary)
      {
         /* TODO - this should be: */
         /* PrepareDataBuffers(BACKUP) */
         /* ar->cmsrl.state = PF_CMSRL_STATE_WBACKUP; */

         ar->cmsrl.state = PF_CMSRL_STATE_BACKUP;
         set_ar_input_data_status (
            net,
            ar,
            false,
            (ar_set->state == PF_AR_SET_STATE_PRIMARY_LOST));
      }
      break;
   case PF_CMSRL_STATE_WBACKUP:
      if (primary)
      {
         if (last_ar_of_ar_set (ar))
         {
            pf_cmsrl_stop_rdht (net, ar_set);
         }
         ar->ar_state = PF_AR_STATE_BACKUP;
         ar->cmsrl.state = PF_CMSRL_STATE_IDLE;
         (void)pf_cmdev_state_ind (net, ar, PNET_EVENT_ABORT);
      }
      break;
   default:
      break;
   }
   os_mutex_unlock (ar_set->lock);
}

/* AR set functions */

static void ar_set_update_state (pnet_t * net, pf_ar_set_t * ar_set)
{
   int n;

   for (n = 0; n < ar_set->size; ++n)
   {
      if (ar_set->contains_ar[n])
      {
         if (
            (ar_set->ar[n]->ar_state == PF_AR_STATE_PRIMARY) &&
            (ar_set->ar[n]->cmsrl.state != PF_CMSRL_STATE_IDLE))
         {
            ar_set->primary_ar = ar_set->ar[n];
            /* The AR set contains an AR in Primary state. */
            if (ar_set->state != PF_AR_SET_STATE_PRIMARY_EXIST)
            {
               ar_set->state = PF_AR_SET_STATE_PRIMARY_EXIST;
               pf_cmsrl_ar_set_state_changed (net, ar_set);
               pf_cmsrl_al_ar_set_state_changed (net, ar_set);
            }
            return;
         }
      }
   }
   /* The AR set contains no AR in Primary state. */
   if (ar_set->state == PF_AR_SET_STATE_PRIMARY_EXIST)
   {
      ar_set->primary_ar = NULL;
      ar_set->state = PF_AR_SET_STATE_PRIMARY_LOST;
      pf_cmsrl_ar_set_state_changed (net, ar_set);
      pf_cmsrl_al_ar_set_state_changed (net, ar_set);
   }
}
