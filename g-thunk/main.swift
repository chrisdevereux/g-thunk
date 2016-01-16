import Foundation

/** Compiler Init **/

let program = Program(
  procedures: [
    Procedure(
      name: "TestProc",
      params: [],
      blocks: [
        SSANode.AddNum(SSANum.Real(10), SSANum.Real(12))
      ]
    )
  ],
  entry: 0
)

let myRenderLoop = compileProgram(program)



/** AudioIO Init **/

let io = audio_io_alloc()

// For now, use a hardcoded renderloop until the compiler produces useful code
// This function should be expressible as:
//
// let sinOsc = (hz: Number, {time: Second}) -> sin(2 * PI * hz * time)
// export sinOsc(440)
//
// The last argument contains the render state.
// In the future, this may be extended to include other kinds of inputs
//
func renderLoop(
  inputs: UnsafePointer<AudioBuffer>,
  outputs: UnsafePointer<AudioBuffer>,
  inputCount: UInt32,
  outputCount: UInt32,
  frame_start: UInt64,
  frame_length: UInt64,
  sampleRate: Float32
) {
  let PI: Float32 = 3.1415926535;
  let pitch: Float32 = 180.0;
  let radians_per_second: Float32 = pitch * 2.0 * PI;
  let seconds_per_frame = 1.0 / sampleRate;
  
  let seconds_offset = seconds_per_frame * Float32(frame_start);
  
  for var i: UInt64 = 0; i < frame_length; i++ {
    let sample: Float32 = sinf(
      (seconds_offset + Float32(i) * seconds_per_frame)
      * radians_per_second
    );
    
    for var chan = 0; chan < Int(outputCount); chan++ {
      let buffer = outputs.advancedBy(chan).memory;
      buffer.data.advancedBy(Int(buffer.step * i)).memory = sample
    }
  }
}

audio_io_set_render_loop(io, renderLoop)
audio_io_start(io)



/** EventLoop Init **/

typealias WatchHandle = UnsafeMutablePointer<uv_fs_event_t>

func handleFileChange(
  handle: WatchHandle,
  filenameBuf: UnsafePointer<CSignedChar>,
  events: CInt,
  status: CInt
) {
  if let filename = String.fromCString(filenameBuf) {
    print("Changed:", filename)
  }
}

let dir = NSFileManager.defaultManager().currentDirectoryPath
print("Watching:", dir)

let loop = uv_default_loop()
let watchHandle = WatchHandle(malloc(sizeof(uv_fs_event_t)));


uv_fs_event_init(loop, watchHandle)
uv_fs_event_start(watchHandle, handleFileChange, dir, 4);

uv_run(loop, UV_RUN_DEFAULT);
