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
pub struct Fsm {
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

impl Fsm {
    pub fn clear_start(&mut self) {
        self.hasstart = false;
    }

    pub fn set_start(&mut self, state: StateId) {
        assert!((state as usize) < self.statecount);

        self.start = state;
        self.hasstart = true;
    }

    pub fn get_start(&self) -> Option<StateId> {
        if self.hasstart {
            assert!((self.start as usize) < self.statecount);
            Some(self.start)
        } else {
            None
        }
    }
}

#[no_mangle]
pub unsafe fn fsm_clearstart(fsm: *mut Fsm) {
    assert!(!fsm.is_null());
    let fsm = &mut *fsm;

    fsm.clear_start();
}

#[no_mangle]
pub unsafe fn fsm_setstart(fsm: *mut Fsm, state: StateId) {
    assert!(!fsm.is_null());
    let fsm = &mut *fsm;

    fsm.set_start(state);
}

#[no_mangle]
pub unsafe fn fsm_getstart(fsm: *const Fsm, out_start: *mut StateId) -> i32 {
    assert!(!fsm.is_null());
    assert!(!out_start.is_null());

    let fsm = &*fsm;
    match fsm.get_start() {
        Some(id) => {
            *out_start = id;
            1
        }

        None => 0
    }
}

#[cfg(test)]
mod tests {
    use std::ptr;
    use super::*;

    // Just here until we can construct Fsm in Rust
    fn dummy_fsm_new() -> Fsm {
        Fsm {
            states: ptr::null_mut(),
            statealloc: 0,
            statecount: 0,
            endcount: 0,
            start: 0,
            hasstart: false,
            capture_info: ptr::null_mut(),
            endid_info: ptr::null_mut(),
            opt: ptr::null(),
        }
    }

    #[test]
    fn sets_and_clears_start() {
        let mut fsm = dummy_fsm_new();

        // FIXME: this sets up an inconsistent state, but we need it to
        // test the assertion inside fsm.set_start().
        fsm.statecount = 1;

        fsm.set_start(0);
        assert_eq!(fsm.get_start(), Some(0));

        fsm.clear_start();
        assert!(fsm.get_start().is_none())
    }

    #[test]
    fn sets_and_clears_start_from_c() {
        let mut fsm = dummy_fsm_new();
        let fsm_ptr = &mut fsm as *mut _;

        // FIXME: this sets up an inconsistent state, but we need it to
        // test the assertion inside fsm_setstart().
        fsm.statecount = 1;

        unsafe {
            let mut n = 0;

            fsm_setstart(fsm_ptr, 0);
            assert_eq!(fsm_getstart(fsm_ptr, &mut n), 1);
            assert_eq!(n, 0);

            fsm_clearstart(fsm_ptr);
            assert_eq!(fsm_getstart(fsm_ptr, &mut n), 0);
        }
    }
}
