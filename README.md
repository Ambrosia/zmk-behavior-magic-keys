# zmk-behavior-magic-keys

This module adds the `magic-keys` behavior to ZMK. This behaviour implements the magic and skip
magic keys as defined by the wonderful [Afterburner](https://blog.simn.me/posts/2025/afterburner/)
keyboard layout by [Simon Zeng](https://simn.me/).

Currently this is a quick and dirty implementation that will remain available under the
`v0.0.1` and `single-instance` git tags.
In the future I will be working on a better, more flexible implementation that makes use of
multiple behaviour instances.

## Usage

To add this module to your ZMK config, add the following remote and project to `config/west.yml`,
updating the path to your preference.

```yaml
manifest:
  remotes:
    - name: ambrosia
      url-base: https://github.com/ambrosia

  projects:
    - name: zmk-behavior-magic-keys
      remote: ambrosia
      revision: single-instance
      path: whereever-you-want
```

## Configuration

Add the following to your keymap in the `behaviors` section. This will make `&magic 0` (magic key)
and `&magic 1` (skip magic key) available to your keymap bindings.

```
magic: magic_keys {
    compatible = "zmk,behavior-magic-keys";
    #binding-cells = <1>;
    magic-rules = <...>;
    skip-magic-rules = <...>;
};
```

You may then fill in the `magic-rules` and `skip-magic-rules` properties with pairs of keys,
where the first key of the pair is the input and the second key of the pair is the output.
If an odd number of keys is provided, the final key will be ignored and therefore repeated as
normal.

## Example: Afterburner

Below is a snippet of a keymap using the [Afterburner](https://blog.simn.me/posts/2025/afterburner/)
keyboard layout, on a Toucan keyboard (3x6+3 split keyboard).

```
/ {
    behaviors {
        magic: magic_keys {
            compatible = "zmk,behavior-magic-keys";
            #binding-cells = <1>;
            magic-rules = <
                A O
                G S
                H Y
                U E
                X T
                Y H
            >;
            skip-magic-rules = <
                A O
                B N
                D T
                F S
                G S
                H Y
                J Y
                K T
                L R
                M K
                O A
                P N
                Q E
                R L
                U E
                V T
                X T
                Y H
                COMMA I
                DOT I
                MINUS I
                SLASH A
                SEMI E
            >;
        };
    };

    keymap {
        compatible = "zmk,keymap";

        base {
            display-name = "base";
            bindings = <
                &none &kp J &kp B &kp G &kp D &kp K   &kp Z      &kp C     &kp O     &kp U    &kp COMMA &mo 4
                &kp Q &kp H &kp N &kp S &kp T &kp M   &magic 0   &magic 1  &kp A     &kp E    &kp I     &kp MINUS
                &none &kp Y &kp P &kp F &kp V &kp X   &kp SQT    &kp W     &kp SLASH &kp SEMI &kp DOT   &none
                                  &mo 1 &kp R &kp L   &kp LSHIFT &kp SPACE &mo 2
            >;
        };
    };
};
```

## Thanks

- Simon Zeng: for the tremendous amount of thought and hard work put into the Afterburner keyboard
  layout!
- urob: For making cool behaviours, I would have been totally lost without these examples!
- ZMK contributors!
