process.stdout.write(
  require('child_process')
  .execSync(`ocamldep -modules ${process.argv[2]}`, {encoding: 'utf8'})
  .trim()
  .replace(/.*: /, '')
  .split(' ')
  .filter(x => x.trim().length > 0)
  .map(x => `${x}.cmo`)
  .join(' ')
);
