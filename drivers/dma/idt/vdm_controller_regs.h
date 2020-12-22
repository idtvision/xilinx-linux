// -----------------------------------------------------------------------------
// 'vdm_controller' Register Definitions
// Revision: 89
// -----------------------------------------------------------------------------
// Generated on 2020-08-14 at 21:53 (UTC) by airhdl version 2020.06.1
// -----------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
// -----------------------------------------------------------------------------

#ifndef VDM_CONTROLLER_REGS_H
#define VDM_CONTROLLER_REGS_H

/* Revision number of the 'vdm_controller' register map */
#define VDM_CONTROLLER_REVISION 89

/* Default base address of the 'vdm_controller' register map */
#define VDM_CONTROLLER_DEFAULT_BASEADDR 0x00000000

/* Register 'version' */
#define VERSION_OFFSET 0x00000000 /* address offset of the 'version' register */

/* Field  'version.value' */
#define VERSION_VALUE_BIT_OFFSET 0 /* bit offset of the 'value' field */
#define VERSION_VALUE_BIT_WIDTH 32 /* bit width of the 'value' field */
#define VERSION_VALUE_BIT_MASK 0xFFFFFFFF /* bit mask of the 'value' field */
#define VERSION_VALUE_RESET 0x0 /* reset value of the 'value' field */

/* Register 'status' */
#define STATUS_OFFSET 0x00000004 /* address offset of the 'status' register */

/* Field  'status.fifo_ready' */
#define STATUS_FIFO_READY_BIT_OFFSET 0 /* bit offset of the 'fifo_ready' field */
#define STATUS_FIFO_READY_BIT_WIDTH 1 /* bit width of the 'fifo_ready' field */
#define STATUS_FIFO_READY_BIT_MASK 0x00000001 /* bit mask of the 'fifo_ready' field */
#define STATUS_FIFO_READY_RESET 0x0 /* reset value of the 'fifo_ready' field */

/* Field  'status.fifo_empty' */
#define STATUS_FIFO_EMPTY_BIT_OFFSET 1 /* bit offset of the 'fifo_empty' field */
#define STATUS_FIFO_EMPTY_BIT_WIDTH 1 /* bit width of the 'fifo_empty' field */
#define STATUS_FIFO_EMPTY_BIT_MASK 0x00000002 /* bit mask of the 'fifo_empty' field */
#define STATUS_FIFO_EMPTY_RESET 0x0 /* reset value of the 'fifo_empty' field */

/* Field  'status.state_idle' */
#define STATUS_STATE_IDLE_BIT_OFFSET 2 /* bit offset of the 'state_idle' field */
#define STATUS_STATE_IDLE_BIT_WIDTH 1 /* bit width of the 'state_idle' field */
#define STATUS_STATE_IDLE_BIT_MASK 0x00000004 /* bit mask of the 'state_idle' field */
#define STATUS_STATE_IDLE_RESET 0x0 /* reset value of the 'state_idle' field */

/* Field  'status.state_post_trigger' */
#define STATUS_STATE_POST_TRIGGER_BIT_OFFSET 3 /* bit offset of the 'state_post_trigger' field */
#define STATUS_STATE_POST_TRIGGER_BIT_WIDTH 1 /* bit width of the 'state_post_trigger' field */
#define STATUS_STATE_POST_TRIGGER_BIT_MASK 0x00000008 /* bit mask of the 'state_post_trigger' field */
#define STATUS_STATE_POST_TRIGGER_RESET 0x0 /* reset value of the 'state_post_trigger' field */

/* Field  'status.tag' */
#define STATUS_TAG_BIT_OFFSET 4 /* bit offset of the 'tag' field */
#define STATUS_TAG_BIT_WIDTH 4 /* bit width of the 'tag' field */
#define STATUS_TAG_BIT_MASK 0x000000F0 /* bit mask of the 'tag' field */
#define STATUS_TAG_RESET 0x0 /* reset value of the 'tag' field */

/* Field  'status.internal_error' */
#define STATUS_INTERNAL_ERROR_BIT_OFFSET 8 /* bit offset of the 'internal_error' field */
#define STATUS_INTERNAL_ERROR_BIT_WIDTH 1 /* bit width of the 'internal_error' field */
#define STATUS_INTERNAL_ERROR_BIT_MASK 0x00000100 /* bit mask of the 'internal_error' field */
#define STATUS_INTERNAL_ERROR_RESET 0x0 /* reset value of the 'internal_error' field */

/* Field  'status.dec_error' */
#define STATUS_DEC_ERROR_BIT_OFFSET 9 /* bit offset of the 'dec_error' field */
#define STATUS_DEC_ERROR_BIT_WIDTH 1 /* bit width of the 'dec_error' field */
#define STATUS_DEC_ERROR_BIT_MASK 0x00000200 /* bit mask of the 'dec_error' field */
#define STATUS_DEC_ERROR_RESET 0x0 /* reset value of the 'dec_error' field */

/* Field  'status.slave_error' */
#define STATUS_SLAVE_ERROR_BIT_OFFSET 10 /* bit offset of the 'slave_error' field */
#define STATUS_SLAVE_ERROR_BIT_WIDTH 1 /* bit width of the 'slave_error' field */
#define STATUS_SLAVE_ERROR_BIT_MASK 0x00000400 /* bit mask of the 'slave_error' field */
#define STATUS_SLAVE_ERROR_RESET 0x0 /* reset value of the 'slave_error' field */

/* Field  'status.ok' */
#define STATUS_OK_BIT_OFFSET 11 /* bit offset of the 'ok' field */
#define STATUS_OK_BIT_WIDTH 1 /* bit width of the 'ok' field */
#define STATUS_OK_BIT_MASK 0x00000800 /* bit mask of the 'ok' field */
#define STATUS_OK_RESET 0x0 /* reset value of the 'ok' field */

/* Field  'status.integrity_error' */
#define STATUS_INTEGRITY_ERROR_BIT_OFFSET 31 /* bit offset of the 'integrity_error' field */
#define STATUS_INTEGRITY_ERROR_BIT_WIDTH 1 /* bit width of the 'integrity_error' field */
#define STATUS_INTEGRITY_ERROR_BIT_MASK 0x80000000 /* bit mask of the 'integrity_error' field */
#define STATUS_INTEGRITY_ERROR_RESET 0x0 /* reset value of the 'integrity_error' field */

/* Register 'control' */
#define CONTROL_OFFSET 0x00000008 /* address offset of the 'control' register */

/* Field  'control.reset' */
#define CONTROL_RESET_BIT_OFFSET 0 /* bit offset of the 'reset' field */
#define CONTROL_RESET_BIT_WIDTH 1 /* bit width of the 'reset' field */
#define CONTROL_RESET_BIT_MASK 0x00000001 /* bit mask of the 'reset' field */
#define CONTROL_RESET_RESET 0x0 /* reset value of the 'reset' field */

/* Field  'control.break_current' */
#define CONTROL_BREAK_CURRENT_BIT_OFFSET 1 /* bit offset of the 'break_current' field */
#define CONTROL_BREAK_CURRENT_BIT_WIDTH 1 /* bit width of the 'break_current' field */
#define CONTROL_BREAK_CURRENT_BIT_MASK 0x00000002 /* bit mask of the 'break_current' field */
#define CONTROL_BREAK_CURRENT_RESET 0x0 /* reset value of the 'break_current' field */

/* Field  'control.break_all' */
#define CONTROL_BREAK_ALL_BIT_OFFSET 2 /* bit offset of the 'break_all' field */
#define CONTROL_BREAK_ALL_BIT_WIDTH 1 /* bit width of the 'break_all' field */
#define CONTROL_BREAK_ALL_BIT_MASK 0x00000004 /* bit mask of the 'break_all' field */
#define CONTROL_BREAK_ALL_RESET 0x0 /* reset value of the 'break_all' field */

/* Field  'control.break_now' */
#define CONTROL_BREAK_NOW_BIT_OFFSET 3 /* bit offset of the 'break_now' field */
#define CONTROL_BREAK_NOW_BIT_WIDTH 1 /* bit width of the 'break_now' field */
#define CONTROL_BREAK_NOW_BIT_MASK 0x00000008 /* bit mask of the 'break_now' field */
#define CONTROL_BREAK_NOW_RESET 0x0 /* reset value of the 'break_now' field */

/* Field  'control.repeat' */
#define CONTROL_REPEAT_BIT_OFFSET 4 /* bit offset of the 'repeat' field */
#define CONTROL_REPEAT_BIT_WIDTH 1 /* bit width of the 'repeat' field */
#define CONTROL_REPEAT_BIT_MASK 0x00000010 /* bit mask of the 'repeat' field */
#define CONTROL_REPEAT_RESET 0x0 /* reset value of the 'repeat' field */

/* Field  'control.incr' */
#define CONTROL_INCR_BIT_OFFSET 5 /* bit offset of the 'incr' field */
#define CONTROL_INCR_BIT_WIDTH 1 /* bit width of the 'incr' field */
#define CONTROL_INCR_BIT_MASK 0x00000020 /* bit mask of the 'incr' field */
#define CONTROL_INCR_RESET 0x0 /* reset value of the 'incr' field */

/* Field  'control.follow' */
#define CONTROL_FOLLOW_BIT_OFFSET 6 /* bit offset of the 'follow' field */
#define CONTROL_FOLLOW_BIT_WIDTH 1 /* bit width of the 'follow' field */
#define CONTROL_FOLLOW_BIT_MASK 0x00000040 /* bit mask of the 'follow' field */
#define CONTROL_FOLLOW_RESET 0x0 /* reset value of the 'follow' field */

/* Field  'control.streaming' */
#define CONTROL_STREAMING_BIT_OFFSET 7 /* bit offset of the 'streaming' field */
#define CONTROL_STREAMING_BIT_WIDTH 1 /* bit width of the 'streaming' field */
#define CONTROL_STREAMING_BIT_MASK 0x00000080 /* bit mask of the 'streaming' field */
#define CONTROL_STREAMING_RESET 0x0 /* reset value of the 'streaming' field */

/* Field  'control.ip' */
#define CONTROL_IP_BIT_OFFSET 8 /* bit offset of the 'ip' field */
#define CONTROL_IP_BIT_WIDTH 16 /* bit width of the 'ip' field */
#define CONTROL_IP_BIT_MASK 0x00FFFF00 /* bit mask of the 'ip' field */
#define CONTROL_IP_RESET 0x0 /* reset value of the 'ip' field */

/* Field  'control.run' */
#define CONTROL_RUN_BIT_OFFSET 31 /* bit offset of the 'run' field */
#define CONTROL_RUN_BIT_WIDTH 1 /* bit width of the 'run' field */
#define CONTROL_RUN_BIT_MASK 0x80000000 /* bit mask of the 'run' field */
#define CONTROL_RUN_RESET 0x0 /* reset value of the 'run' field */

/* Register 'inst_cnt' */
#define INST_CNT_OFFSET 0x0000000C /* address offset of the 'inst_cnt' register */

/* Field  'inst_cnt.value' */
#define INST_CNT_VALUE_BIT_OFFSET 0 /* bit offset of the 'value' field */
#define INST_CNT_VALUE_BIT_WIDTH 32 /* bit width of the 'value' field */
#define INST_CNT_VALUE_BIT_MASK 0xFFFFFFFF /* bit mask of the 'value' field */
#define INST_CNT_VALUE_RESET 0x0 /* reset value of the 'value' field */

/* Register 'ip' */
#define IP_OFFSET 0x00000010 /* address offset of the 'ip' register */

/* Field  'ip.value' */
#define IP_VALUE_BIT_OFFSET 0 /* bit offset of the 'value' field */
#define IP_VALUE_BIT_WIDTH 32 /* bit width of the 'value' field */
#define IP_VALUE_BIT_MASK 0xFFFFFFFF /* bit mask of the 'value' field */
#define IP_VALUE_RESET 0x0 /* reset value of the 'value' field */

/* Register 'last_addr' */
#define LAST_ADDR_OFFSET 0x00000020 /* address offset of the 'last_addr' register */

/* Field  'last_addr.value' */
#define LAST_ADDR_VALUE_BIT_OFFSET 0 /* bit offset of the 'value' field */
#define LAST_ADDR_VALUE_BIT_WIDTH 32 /* bit width of the 'value' field */
#define LAST_ADDR_VALUE_BIT_MASK 0xFFFFFFFF /* bit mask of the 'value' field */
#define LAST_ADDR_VALUE_RESET 0x0 /* reset value of the 'value' field */

/* Register 'cur_addr' */
#define CUR_ADDR_OFFSET 0x00000024 /* address offset of the 'cur_addr' register */

/* Field  'cur_addr.value' */
#define CUR_ADDR_VALUE_BIT_OFFSET 0 /* bit offset of the 'value' field */
#define CUR_ADDR_VALUE_BIT_WIDTH 32 /* bit width of the 'value' field */
#define CUR_ADDR_VALUE_BIT_MASK 0xFFFFFFFF /* bit mask of the 'value' field */
#define CUR_ADDR_VALUE_RESET 0x0 /* reset value of the 'value' field */

/* Register 'program' */
#define PROGRAM_OFFSET 0x00001000 /* address offset of the 'program' register */
#define PROGRAM_DEPTH 1024 /* depth of the 'program' memory, in elements */

/* Field  'program.value' */
#define PROGRAM_VALUE_BIT_OFFSET 0 /* bit offset of the 'value' field */
#define PROGRAM_VALUE_BIT_WIDTH 32 /* bit width of the 'value' field */
#define PROGRAM_VALUE_BIT_MASK 0xFFFFFFFF /* bit mask of the 'value' field */
#define PROGRAM_VALUE_RESET 0x0 /* reset value of the 'value' field */

#endif  /* VDM_CONTROLLER_REGS_H */
