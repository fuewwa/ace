#!/usr/bin/env python3
import sys
import pygame

WIDTH = 480
HEIGHT = 160
PADDING = 20
BG_COLOR = (30, 30, 30)
TEXT_COLOR = (255, 80, 80)
HINT_COLOR = (150, 150, 150)


def wrap_text(text, font, max_width):
    words = text.split(" ")
    lines = []
    current = ""
    for word in words:
        candidate = current + (" " if current else "") + word
        if font.size(candidate)[0] <= max_width:
            current = candidate
        else:
            if current:
                lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines


def show_error(message):
    pygame.init()
    pygame.font.init()

    screen = pygame.display.set_mode((WIDTH, HEIGHT))
    pygame.display.set_caption("acerror")
    font = pygame.font.SysFont(None, 28)
    clock = pygame.time.Clock()

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN and event.key in (pygame.K_ESCAPE, pygame.K_RETURN):
                running = False

        screen.fill(BG_COLOR)

        lines = wrap_text(message, font, WIDTH - 2 * PADDING)
        y = PADDING
        for line in lines:
            surface = font.render(line, True, TEXT_COLOR)
            screen.blit(surface, (PADDING, y))
            y += surface.get_height() + 6

        hint = font.render("Press Enter or Esc to close", True, HINT_COLOR)
        screen.blit(hint, (PADDING, HEIGHT - 30))

        pygame.display.flip()
        clock.tick(30)

    pygame.quit()


def main():
    message = " ".join(sys.argv[1:]) if len(sys.argv) > 1 else "Unknown error"
    show_error(message)


if __name__ == "__main__":
    main()
