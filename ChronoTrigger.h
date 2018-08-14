#ifndef CHRONOTRIGGER_H
#define CHRONOTRIGGER_H

#include <Arduino.h>
#include "OnyxTypeAliases.h"


/* *TODO* */
/* - Add _threshold_ for softrealtime vs longoperation */
/* - For doing a long operation one must first `request_long_op_state()` */
/* - When doing softreal timing sensitive stuff, must do `set_softreal_phase()` */
/* - and clear it when done... (it incs/decs a counter to cover for several) */
/* - this way no us-sensitive ops are intervened */
/* - if a long op HAS to be done (doing _that_ is timing-sensitive), it can be */
/*   forced - the softreal branches can see that the promise has been broken via  */
/*   a check - it's up to them to check. They may be better off just failing if */
/*   their regular timing errors fail checking is stable enough (SHOULD BE!) */

/* *TODO* *DOUBLE* *UP* *TODO* */
/* - the api should be re-vamped */
/* - use `go_next` EVERY TIME! */
/*    - IF NOT SET AFTER RETURN FROM UPDATE - ERROR FATAL */
/* - add `on_timeout_go` for automated transition - mtime is reset each time it's called */
/* - if `go_next` is called with _new state_ — different from previous — then on_timeout is cleared */
/*
   where-to-go() -> ChronoState
   go-next(s ChronoState) -> Void
   on-timeout-go(s ChronoState, t TimeSpan,

*/

// *TODO* - "over_extended_duration" - passing on timing delta (if sleep took
// longer than planned (always does to some extend) if that should be subtracted for the next defer
// This requires that we're not already running at top speed, cause we can't
// compensate a slower than wanted step with a faster than possible one...



// *TODO* styr upp dessa motherfuckers
template <typename VT>
class CTReadValueWrapper {
   virtual fn is_ready() -> Bool   =  0;
   virtual fn get() -> VT          =  0;
};


typedef I8  ChronoState;

/* template <Bool TrackOverExtend = false> */ // This will highly likely be optimized away by the compiler anyway when not used - so fuck it
class ChronoTrigger {
  private:
   ChronoState state             = Default;
   ChronoState scheduled_state   = NoWhere;

   ChronoState timeout_state     = NoWhere;


   TimeSpan    transition_delay  = 0;        // We can't just calculate scheduled-time, since we can re-schedule, and don't know the previous schedules duration then.
   TimeStamp   last_ts           = 0;
   TimeSpan    over_extended     = 0;
   Bool        in_sleeping_wait  = false;

  public:
   static const ChronoState NoWhere    = 0;
   static const ChronoState Default    = -1;
   static const ChronoState Main       = -2;
   static const ChronoState Sleeping   = -128;

  public:
   virtual fn update() -> Void   = 0;        // _Essential_: Update state (do stuff!)
   // NOTE!
   // These has been removed as "requirements", since:
   //    - they _aren't_ really required!
   /* virtual fn is_ready() -> Bool = 0;        // Is it ready with usable results? */
   /* virtual fn log() -> Void      = 0;        // For simplified unified debugging */

   fn where_to_go() -> ChronoState {
      if (scheduled_state == NoWhere) {
         return state;
      }

      TimeSpan t_delta = timestamp_() - last_ts;
      /* Since we're unsigned, and it's two's complement, this is moot - if (t_delta < 0) t_delta += 65536; // *TODO* preferably use `std::numerical_limits<T>::MAX` */

      if (t_delta > transition_delay) {
         /* if (TrackOverExtend) { */
         over_extended = t_delta - transition_delay; // *TODO* double check the overflow characteristics
         /* } */
         state = scheduled_state;
         cancel_scheduled_state();
         return state;

      } else if (in_sleeping_wait) {
         return Sleeping;

      } else {
         return state;
      }
   }

   inl is_sleeping() -> Bool {
      return where_to_go() == Sleeping; //in_sleeping_wait;
   }


   // // // //
   // // // //
   fn go_next__tko(ChronoState state_, TimeSpan resting_time_ = 0) -> Void {
      if (state != state_) {
         clear_timeout__tko();
         state = state_;
      }
      touch_();
   }

   fn on_timeout__tko(ChronoState state_, TimeSpan timeout_) -> Void {

   }

   fn clear_timeout__tko() -> Void {

   }
   // // // //
   // // // //
   // // // //



   fn go_now(ChronoState state_, Bool keep_scheduled = false) -> Void {
      go_next(state_, keep_scheduled);
      update();
   }

   fn go_next(ChronoState state_, Bool keep_scheduled = false) -> Void {
      state = state_;
      touch_();

      if (keep_scheduled == false)
         cancel_scheduled_state();
   }

   inl go_next_if(ChronoState state_, ChronoState check_state_equals) -> Void {
      if (state == check_state_equals) {
         go_next(state_); // Add "keep_scheduled" when/if needed
      }
   }

   fn go_after(ChronoState state_, TimeSpan delay, Bool reset_timestamp) -> Void {
      // *TODO* add a check when debug mode:
      // - a too optimistically expected short sleep likely can't be fulfilled
      // - either stick with blocking style
      // - or add additional update after another swift instance-update
      //    - requires benchmarking and puzzling time-sharers with _care_
      in_sleeping_wait = false;
      transition_delay = delay;
      if (reset_timestamp || state_ == NoWhere || scheduled_state != state_) {
         touch_();
      }
      scheduled_state = state_;
   }

   inl go_after_sleep(ChronoState state_, TimeSpan delay) -> Void {
      go_after(state_, delay, true);
      in_sleeping_wait = true;
   }

   inl sleep(TimeSpan delay) -> Void {
      go_after_sleep(state, delay);
   }

   fn cancel_scheduled_state() -> Void {
      scheduled_state = NoWhere;
      transition_delay = 0;
      touch_();
   }

  protected:
   inl set_state_(ChronoState state_) -> Void {
      state = state_;
   }

   inl get_state_() -> ChronoState {
      return state;
   }

   inl timestamp() -> TimeStamp {
      return timestamp_();
   }

  private:
   inl timestamp_() -> TimeStamp {
      return micros();
   }

   inl touch_() -> Void {
      last_ts = timestamp_();
   }

};


#endif
