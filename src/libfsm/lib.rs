mod fsm;

// just here to prove linking works
#[no_mangle]
pub extern "C" fn fsm_noop() {
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
