// Copyright 2021, The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use keystore2_selinux as selinux;
use nix::sys::wait::{waitpid, WaitStatus};
use nix::unistd::{
    close, fork, pipe as nix_pipe, read, setgid, setuid, write, ForkResult, Gid, Uid,
};
use serde::{Serialize, de::DeserializeOwned};
use std::os::unix::io::RawFd;

#[derive(Debug)]
enum Error {
    SomethingWentWrong,
    Errno(nix::Error),
    SetGid(nix::Error),
    SetUid(nix::Error),
    Setexeccon,
    Setcon,
    Execv(nix::Error),
}

fn transition(se_context: selinux::Context, uid: Uid, gid: Gid) -> Result<(), Error> {
    setgid(gid).map_err(Error::SetGid)?;
    setuid(uid).map_err(Error::SetUid)?;

    selinux::setcon(&se_context).expect("Failed to setcon.");
    Ok(())
}

struct PipeReader(RawFd);

impl PipeReader {
    pub fn read_all(&self) -> Result<Vec<u8>, nix::Error> {
        let mut buffer = [0u8; 128];
        let mut result = Vec::<u8>::new();
        loop {
            let bytes = read(self.0, &mut buffer)?;
            if bytes == 0 {
                return Ok(result);
            }
            result.extend_from_slice(&buffer[0..bytes]);
        }
    }
}

impl Drop for PipeReader {
    fn drop(&mut self) {
        close(self.0).expect("Failed to close reader pipe fd.");
    }
}

struct PipeWriter(RawFd);

impl PipeWriter {
    pub fn write(&self, data: &[u8]) -> Result<usize, nix::Error> {
        write(self.0, data)
    }
}

impl Drop for PipeWriter {
    fn drop(&mut self) {
        close(self.0).expect("Failed to close writer pipe fd.");
    }
}

fn pipe() -> Result<(PipeReader, PipeWriter), nix::Error> {
    let (read_fd, write_fd) = nix_pipe()?;
    Ok((PipeReader(read_fd), PipeWriter(write_fd)))
}

pub fn run_as<F, R>(se_context: selinux::Context, uid: Uid, gid: Gid, f: F) -> R
where
    R: Serialize + DeserializeOwned,
    F: 'static + Send + FnOnce() -> R,
{
    let (reader, writer) = pipe().expect("Failed to create pipe.");

    match unsafe { fork() } {
        Ok(ForkResult::Parent { child, .. }) => {
            drop(writer);
            let status = waitpid(child, None).expect("Failed while waiting for child.");
            if let WaitStatus::Exited(_, 0) = status {
                // Child exited successfully.
                let serialized_result = reader
                    .read_all()
                    .expect("Failed to read result from child.");
                serde_cbor::from_slice(&serialized_result)
                    .expect("Failed to deserialize result.")
            } else {
                panic!("Child did not exit as expected {:?}", status);
            }
        }
        Ok(ForkResult::Child) => {

            transition(se_context, uid, gid).expect("Failed to transition to target identity.");

            let result = f();
            let vec = serde_cbor::to_vec(&result).expect("Result serialization failed");
            writer
                .write(&vec)
                .expect("Failed to send serialized result to parent.");
            std::process::exit(0);
        }
        Err(errno) => {
            panic!("Failed to fork: {:?}", errno);
        }
    }
}
