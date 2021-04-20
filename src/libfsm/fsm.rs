//! Representation of a Finite State Machine.

use libc::c_void;
use std::slice;

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

const ENDCOUNT_MAX: usize = usize::MAX;

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

    fn states_as_mut_slice(&self) -> &mut [State] {
        unsafe { slice::from_raw_parts_mut(self.states, self.statecount) }
    }

    pub fn set_end(&mut self, state: StateId, end: bool) {
        assert!((state as usize) < self.statecount);

        {
            // Temporary scope so the `s` mutable borrow terminates before
            // we frob `self.endcount` below.

            let states = self.states_as_mut_slice();
            let s = &mut states[state as usize];

            if s.end == end {
                return;
            } else {
                s.end = end;
            }
        }

        if end {
            assert!(self.endcount < ENDCOUNT_MAX);
            self.endcount += 1;
        } else {
            assert!(self.endcount > 0);
            self.endcount -= 1;
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

        None => 0,
    }
}

#[no_mangle]
pub unsafe fn fsm_setend(fsm: *mut Fsm, state: StateId, end: i32) {
    assert!(!fsm.is_null());
    let fsm = &mut *fsm;

    let end = if end != 0 { true } else { false };

    fsm.set_end(state, end)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::ptr;

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

    #[test]
    fn set_end_works() {
        let mut fsm = dummy_fsm_new();

        fn make_state() -> State {
            State {
                end: false,
                has_capture_actions: false,
                visited: false,
                edges: ptr::null_mut(),
                epsilons: ptr::null_mut(),
            }
        }

        let mut states = vec![make_state(), make_state(), make_state()];

        fsm.states = states.as_mut_ptr();
        fsm.statecount = 3;
        fsm.statealloc = 3;

        fsm.set_end(1, true); // yay aliased mutability
        assert_eq!(fsm.endcount, 1);
        assert!(states[1].end);

        fsm.set_end(1, false);
        assert_eq!(fsm.endcount, 0);
        assert!(!states[1].end);
    }
}
