//! Representation of a Finite State Machine.

use libc::c_void;

// Keep in sync with include/fsm.h:fsm_state_t
type StateId = u32;

// Opaque pointer to struct edget_set
type EdgeSet = *mut c_void;

// Opaque pointer to struct state_set
type StateSet = *mut c_void;

// Opaque pointer to struct fsm_capture_info
type CaptureInfo = *mut c_void;

// Opaque pointer to struct endid_info
type EndIdInfo = *mut c_void;

// Opaque pointer to struct fsm_options
type Options = *const c_void;

/// One state in a `Fsm`'s array of states.
// Keep in sync with interanl.h:struct fsm_state
#[repr(C)]
struct State {
    end: bool,

    /// If false, then this state has no need for checking the fsm->capture_info struct.
    has_capture_actions: bool,

    /// Meaningful within one particular transformation only.
    visited: bool,

    edges: EdgeSet,
    epsilons: StateSet,
}

/// Finite State Machine.
// Keep in sync with interanl.h:struct fsm
#[repr(C)]
struct Fsm {
    /// Array.
    states: *mut State,

    /// Number of elements allocated.
    statealloc: usize,

    /// Number of elements populated.
    statecount: usize,

    endcount: usize,

    start: StateId,
    hasstart: bool,

    capture_info: CaptureInfo,
    endid_info: EndIdInfo,
    opt: Options,
}
