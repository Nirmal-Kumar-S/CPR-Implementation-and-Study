#include "pipeline.h"


void pipeline_t::squash_complete(reg_t jump_PC) {
	unsigned int i, j;

	//////////////////////////
	// Fetch Stage
	//////////////////////////
  
	FetchUnit->flush(jump_PC);

	//////////////////////////
	// Decode Stage
	//////////////////////////

	for (i = 0; i < fetch_width; i++) {
		DECODE[i].valid = false;
	}

	//////////////////////////
	// Rename1 Stage
	//////////////////////////

	FQ.flush();

	//////////////////////////
	// Rename2 Stage
	//////////////////////////

	for (i = 0; i < dispatch_width; i++) {
		RENAME2[i].valid = false;
	}

        //
        // FIX_ME #17c
        // Squash the renamer.
        //

        // FIX_ME #17c BEGIN
		REN->squash();
		// ---------------------------------- //
		instr_renamed_since_last_checkpoint = 0;
		// ---------------------------------- //
        // FIX_ME #17c END


	//////////////////////////
	// Dispatch Stage
	//////////////////////////

	for (i = 0; i < dispatch_width; i++) {
		DISPATCH[i].valid = false;
	}

	//////////////////////////
	// Schedule Stage
	//////////////////////////

	IQ.flush();

	//////////////////////////
	// Register Read Stage
	// Execute Stage
	// Writeback Stage
	//////////////////////////

	for (i = 0; i < issue_width; i++) {
		Execution_Lanes[i].rr.valid = false;
		for (j = 0; j < Execution_Lanes[i].ex_depth; j++)
		   Execution_Lanes[i].ex[j].valid = false;
		Execution_Lanes[i].wb.valid = false;
	}

	LSU.flush();
}


void pipeline_t::selective_squash(uint64_t squash_mask) {
	unsigned int i, j;

	// Squash all instructions in the Decode through Dispatch Stages.

	// Decode Stage:
	for (i = 0; i < fetch_width; i++) {
		DECODE[i].valid = false;
	}

	// Rename1 Stage:
	FQ.flush();

	// Rename2 Stage:
	for (i = 0; i < dispatch_width; i++) {
		RENAME2[i].valid = false;
	}

	// Dispatch Stage:
	for (i = 0; i < dispatch_width; i++) {
		DISPATCH[i].valid = false;
	}

	// Selectively squash instructions after the branch, in the Schedule through Writeback Stages.

	// Schedule Stage:
	IQ.squash(squash_mask);

	for (i = 0; i < issue_width; i++) {
		// Register Read Stage:
		if (Execution_Lanes[i].rr.valid && BIT_IS_ONE(squash_mask, Execution_Lanes[i].rr.chkpt_id)) {
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].rr.index].A_phys_reg);
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].rr.index].B_phys_reg);
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].rr.index].D_phys_reg);
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].rr.index].C_phys_reg);
			Execution_Lanes[i].rr.valid = false;
		}

		// Execute Stage:
		for (j = 0; j < Execution_Lanes[i].ex_depth; j++) {
			if (Execution_Lanes[i].ex[j].valid && BIT_IS_ONE(squash_mask, Execution_Lanes[i].ex[j].chkpt_id)) {
				REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].ex[j].index].A_phys_reg);
				REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].ex[j].index].B_phys_reg);
				REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].ex[j].index].D_phys_reg);
				REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].ex[j].index].C_phys_reg);
				Execution_Lanes[i].ex[j].valid = false;
			}
		}

		// Writeback Stage:
		if (Execution_Lanes[i].wb.valid && BIT_IS_ONE(squash_mask, Execution_Lanes[i].wb.chkpt_id)) {
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].wb.index].A_phys_reg);
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].wb.index].B_phys_reg);
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].wb.index].D_phys_reg);
			REN->dec_usage_counter(PAY.buf[Execution_Lanes[i].wb.index].C_phys_reg);
			Execution_Lanes[i].wb.valid = false;
		}
	}
}
