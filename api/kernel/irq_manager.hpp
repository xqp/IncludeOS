// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

#ifndef KERNEL_IRQ_MANAGER_HPP
#define KERNEL_IRQ_MANAGER_HPP

#include <delegate>
#include <membitmap>

#include "os.hpp"
#include "../hw/pic.hpp"

// From osdev
struct IDTDescr {
  uint16_t offset_1;  // offset bits 0..15
  uint16_t selector;  // a code segment selector in GDT or LDT
  uint8_t  zero;      // unused, set to 0
  uint8_t  type_attr; // type and attributes, see below
  uint16_t offset_2;  // offset bits 16..31
};

struct idt_loc {
  uint16_t limit;
  uint32_t base;
}__attribute__((packed));

extern "C" {
  void irq_default_handler();
}

/** A class to manage interrupt handlers

    Anyone can subscribe to IRQ's, but they will be indirectly called via the
    Deferred Procedure Call / Callback system, i.e. when the system is in a
    wait-loop with nothing else to do.

    NOTES:
    * All IRQ-callbacks are in charge of calling End-Of-Interrupt - eoi.
    Why? Because this makes it possible to prevent further interrupts until
    a condition of your choice is met. And, interrupts are costly as they
    always cause vm-exit.

    * IRQ-numbering: 0 or 32?

    @TODO: Remove all dependencies on old SanOS code. In particular, eoi is now in global scope
*/
class IRQ_manager {
public:
  typedef void (*intr_func) ();
  using irq_delegate = delegate<void()>;

  static constexpr uint8_t IRQ_BASE  =  32;
  static constexpr size_t  IRQ_LINES = 128;


  /**
   *  Enable an IRQ line
   *
   *  If no handler is set a default will be used
   *
   *  @param irq: the IRQ to enable
   */
  void enable_irq(uint8_t irq);

  /**
   *  Directly set an IRQ handler in IDT
   *
   *  @param irq:           The IRQ to handle
   *  @param function_addr: A proper IRQ handler
   *
   *  @warning {
   *    This has to be a function that properly returns with `iret`.
   *    Failure to do so will keep the interrupt from firing and cause a
   *    stack overflow or similar badness.
   *  }
   */
  void set_handler(uint8_t irq, intr_func func);

  /** Get handler from inside the IDT. */
  intr_func get_handler(uint8_t irq);

  /**
   *  Subscribe to an IRQ

   *  @param irq: The IRQ to subscribe to
   *  @param del: A delegate to attach to the IRQ DPC-system

   *  The delegate will be called a.s.a.p. after @param irq gets triggered
   *
   *  @warning The delegate is responsible for signalling a proper EOI
   *
   *  @todo Implies enable_irq(irq)?
   *
   *  @todo Create a public member IRQ_manager::eoi for delegates to use
   */
  void subscribe(uint8_t irq, irq_delegate del);

  /**
   *  End of Interrupt
   *
   *  Indicate to the IRQ-controller that the IRQ is handled, allowing new irq.
   *
   *  @param irq: The interrupt number
   *
   *  @note Until this is called, no furter IRQ's will be triggered on this line
   *
   *  @warning This function is only supposed to be called inside an IRQ-handler
   */
  static void eoi(uint8_t irq);

  
  uint8_t get_next_msix_irq();


  void register_interrupt(uint8_t vector);
  
  irq_delegate get_intr_handler(uint8_t vector) {
    return irq_delegates_[vector];
  }

private:
  IDTDescr     idt[IRQ_LINES];
  bool         idt_is_set                {false};
  irq_delegate irq_delegates_[IRQ_LINES];
  int32_t      irq_counters_[IRQ_LINES]  {0};
  
  int timer_interrupts {0};
  
  MemBitmap  irq_subs;
  MemBitmap  irq_pend;
  MemBitmap  irq_todo;
  
  static const char       default_attr {static_cast<char>(0x8e)};
  static const uint16_t   default_sel  {0x8};

  /** STI */
  void enable_interrupts();

  /**
   *  Create an IDT-gate
   *
   *  Use "set_handler" for a simpler version using defaults
   */
  void create_gate(IDTDescr* idt_entry,
                   void (*function_addr)(),
                   uint16_t segment_sel,
                   char attributes);
  
  /** The OS will call the following : */
  friend class OS;
  friend void ::irq_default_handler();

  /** Initialize. Only the OS can initialize the IRQ manager */
  static void init();

  void bsp_init();
  
  /** Notify all delegates waiting for interrupts */
  void notify();

}; //< IRQ_manager

// IDT manager for bootstrap processor
extern IRQ_manager bsp_idt;

#endif //< KERNEL_IRQ_MANAGER_HPP
