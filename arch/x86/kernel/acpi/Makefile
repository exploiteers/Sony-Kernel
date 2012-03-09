subdir-				:= realmode

obj-$(CONFIG_ACPI)		+= boot.o
obj-$(CONFIG_ACPI_SLEEP)	+= sleep.o wakeup_rm.o wakeup_$(BITS).o
obj-$(CONFIG_X86_INTEL_CE_ACPI_TABLES) += intel_ce_tables.o

ifneq ($(shell iasl -h 2> /dev/null),)
quiet_cmd_iasl = IASL    $@
      cmd_iasl = iasl -vi -vs -vr $< && \
                 cat $$(dirname $<)/$$(basename $< .asl).aml | scripts/bin2c > $@

quiet_cmd_cpp_asl_S = CPP     $@
      cmd_cpp_asl_S = $(CPP) -P -C -o $@ $<

$(obj)/intel_ce_DSDT.hex: $(obj)/intel_ce_DSDT.asl
	$(call if_changed,iasl)

$(obj)/intel_ce_DSDT.asl: $(src)/intel_ce_DSDT.asl.S
	$(call if_changed,cpp_asl_S)

clean-files += intel_ce_DSDT.aml intel_ce_DSDT.asl

$(obj)/intel_ce_tables.o: $(src)/intel_ce_DSDT.hex
endif # iasl exists

ifneq ($(CONFIG_ACPI_PROCESSOR),)
obj-y				+= cstate.o
endif

$(obj)/wakeup_rm.o:    $(obj)/realmode/wakeup.bin

$(obj)/realmode/wakeup.bin: FORCE
	$(Q)$(MAKE) $(build)=$(obj)/realmode

