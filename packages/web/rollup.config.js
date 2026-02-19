import copy from 'rollup-plugin-copy';

export default {
  input: 'dist/index.js',
  output: [
    {
      file: 'dist/index.js',
      format: 'es',
      sourcemap: true,
    },
    {
      file: 'dist/index.cjs',
      format: 'cjs',
      sourcemap: true,
      exports: 'named'
    }
  ],
  plugins: [
    copy({
      targets: [
        {
          src: '../../bindings/web/lib/anychat.js',
          dest: 'lib'
        },
        {
          src: '../../bindings/web/lib/anychat.wasm',
          dest: 'lib'
        }
      ],
      hook: 'buildStart'
    })
  ],
  external: []
};
