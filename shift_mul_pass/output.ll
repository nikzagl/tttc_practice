; ModuleID = 'input.ll'
source_filename = "input.ll"

define i32 @test(i32 %a) {
  %1 = shl i32 %a, 3
  ret i32 %1
}
